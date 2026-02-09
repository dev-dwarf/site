#define LF_IMPL
#include "lf.h"

enum TextStyle  {
  NONE,
  BOLD, ITALIC, STRUCK, CODE_INLINE,
  LINK, IMAGE, EXPLAIN,
  TABLE_CELL,
  TEXT_STYLES, SKIP
};

typedef struct Text Text;
struct Text {
  Text *next;
  Text *child;
  enum TextStyle type;
  str s;
};

enum BlockType {
  PARAGRAPH = 1,
  HEADING, RULE, CODE,
  TABLE,
  QUOTE, ORD_LIST, UN_LIST,
};

typedef struct Block Block;
struct Block {
  struct Block *next;
  enum BlockType type;
  s32 num;
  str id;
  str title;
  Text* text;
};

Block* push_block(Arena *a, Block *b, enum BlockType type, str *line);
Block* parse_md(Arena *a, str input) {
  Block *first = Arena_struct_zero(a, Block);
  Block *b = first;

  s32 code_block = 0;
  while (input.len > 0) {
    str raw = str_cut_char(&input, '\n');
    str line = str_skip_whitespace(raw);

    if (str_startl(line, "```")) { // special case for code blocks
      if (b->type != CODE) {
        b = push_block(a, b, CODE, 0);
        b->id = str_trim_whitespace(str_skip(line, 3));
        b->num = code_block++;
      } else {
        b = push_block(a, b, 0, 0);
      }

    } else if (b->type == CODE) {
      b = push_block(a, b, CODE, &raw);

    } else if (line.len == 0) {
      b = push_block(a, b, 0, 0);

    } else if (str_startl(line, "---")) {
      b = push_block(a, b, RULE, 0);

    } else if (str_startl(line, "- ")) { 
      // TODO(lf) UN_LIST and ORD_LIST dont support nested lists
      line = str_skip(line, 2);
      b = push_block(a, b, UN_LIST, &line);

    } else if (char_is_num(line.str[0]) && line.str[1] == '.' && line.str[2] == ' ') {
      line = str_skip(line, 3);
      b = push_block(a, b, ORD_LIST, &line);

    } else if (str_startl(line, "!|")) {
      line = str_skip(line, 2);

      if (b->type != TABLE) {
        s64 loc = str_find_char(line, '|');
        str id = str_first(line, loc);
        line = str_skip(line, loc);
        b = push_block(a, b, TABLE, &line);
        b->id = id;
      } else {
        b = push_block(a, b, TABLE, &line);
      }

    } else if (str_startl(line, "> ")) {
      line = str_skip(line, 2);
      b = push_block(a, b, QUOTE, &line);

    } else if (line.str[0] == '#') {
      b = push_block(a, b, 0, 0);
      while (line.str[b->num] == '#') {
        b->num++;
      }
      line = str_skip(line, b->num);
      b->id = line;
      b->id.len = str_find_char(b->id, ' ');
      line = str_skip(line, b->id.len);
      b = push_block(a, b, HEADING, &line);

    } else {
      b = push_block(a, b, PARAGRAPH, &line);
    }

  }

  return first;
}

bool match_prefix(str *s, char *pre, s32 *tok_len) {
  bool match = true;
  s32 i = 0;
  for (; i < s->len && pre[i]; i++) {
    if (s->str[i] != pre[i]) {
      match = false;
      break;
    }
  }
  *s = match? str_skip(*s, i) : *s;
  *tok_len = match? i : 0; 
  return match && !pre[i];
}

// returns remaining string to be parsed
str parse_inline(Arena *a, Text *p) {
  str s = p->s;
  s32 tok_len = 0;
  enum TextStyle stop = p->type;

  if (stop == LINK || stop == IMAGE || stop == EXPLAIN) {
    s64 loc = str_find_char(s, ')');
    ASSERT(loc >= 0, "parenthesis must close!");
    p->s = str_first(s, loc);
    return str_skip(s, loc+1);
  }

  while (s.len > 0) {
    while (s.len > 0 && (char_is_whitespace(s.str[0]) || char_is_alphanum(s.str[0]))) {
      s = str_skip(s, 1);
    }

    enum TextStyle tok = NONE;
    if (match_prefix(&s, "\\", &tok_len)) { tok = SKIP; } 
    else if (match_prefix(&s, "**", &tok_len)) { tok = BOLD; }
    else if (match_prefix(&s, "*", &tok_len))  { tok = ITALIC; }
    else if (match_prefix(&s, "~~", &tok_len)) { tok = STRUCK; }
    else if (match_prefix(&s, "`", &tok_len))  { tok = CODE_INLINE; }
    else if (match_prefix(&s, "|", &tok_len))  { tok = TABLE_CELL; }
    else if (match_prefix(&s, "@(", &tok_len)) { tok = LINK; }
    else if (match_prefix(&s, "!(", &tok_len)) { tok = IMAGE; }
    else if (match_prefix(&s, "?(", &tok_len)) { tok = EXPLAIN; }
    else { s = str_skip(s, 1); }

    if (tok != NONE) {
      if (tok == stop) {
        break;
      } else if (p->type == SKIP) {
        p->type = NONE;
        tok_len = 0;
        break;
      } else {
        // Finish current 
        p->s = str_first(p->s, (s.str - p->s.str) - tok_len);

        p->child = Arena_struct_zero(a, Text); 
        p->child->type = tok;
        p->child->s = s;
        s = parse_inline(a, p->child);

        p->child->next = Arena_struct_zero(a, Text);
        p = p->child->next;
        p->type = NONE;
        p->s = s;
      }
    }
  }

  p->s = str_first(p->s, MAX(0, (s.str - p->s.str) - tok_len));
  ASSERT(p->s.len >= 0, "Invalid length %d!", (s32) p->s.len);

  return s;
}

Block* push_block(Arena *a, Block *b, enum BlockType type, str *line) {
  if (b->type != type && b->type) {
    b = (b->next = Arena_struct_zero(a, Block));
  }
  b->type = type;

  if (line) {
    Text *next = Arena_struct_zero(a, Text);

    Text *last = b->text;
    if (!last) {
      b->text = next;
    } else {
      while (last->next) {
        last = last->next;
      }
      last->next = next;
    }

    next->s = *line;

    if (b->type != CODE) {
      parse_inline(a, next);
    }
  }

  return b;
}

// REF: https://nullprogram.com/blog/2023/02/13/
typedef struct Buf Buf;
struct Buf { 
  u8 *buf;
  s32 len;
  s32 cap;
  s32 fd;
  s32 err;
};
void append(Buf *b, u8 *data, s32 len) {
  s32 avail = b->cap - b->len;
  s32 amount = b->err? 0 : CLAMP(len, 0, avail);
  for (s32 i = 0; i < amount; i++) {
    b->buf[b->len+i] = data[i];
  }
  b->len += amount;
  b->err |= amount < len;
}
#define append_strl(b, sl) append(b, (u8*)sl, sizeof(sl"")-1)
void append_str(Buf *b, str s) { append(b, s.str, s.len); }

void append_html_inline(Buf *out, Text *t) {
  const str tags[TEXT_STYLES] = {
  [BOLD] = strl("<b>"), 
  [ITALIC] = strl("<em>"), 
  [STRUCK] = strl("<s>"),
  [CODE_INLINE] = strl("<code>"),
  [TABLE_CELL] = strl("<td>"),
  };
  for (; t; t = t->next) {
    str s = t->s;
    if (t->type == LINK) {
      append_strl(out, "<a href='");
      append_str(out, str_cut_char(&s, ' '));
      append_strl(out, "'>");
      append_str(out, s);
      append_strl(out, "</a>");
    } else if (t->type == EXPLAIN) {
      append_strl(out, "<abbr title=\"");
      append_str(out, str_cut_char(&s, ','));
      append_strl(out, "\">");
      append_str(out, s);
      append_strl(out, "</abbr>");
    } else if (t->type == IMAGE) {
      if (str_endl(s, ".mp4")) {
        append_strl(out, "<video controls><source src='");
        append_str(out, s);
        append_strl(out, "' type='video/mp4'></video>");
      } else {
        append_strl(out, "<img src='");
        append_str(out, s);
        append_strl(out, "'>");
      }
    } else {
      str tag = tags[t->type]; 
      if (tag.len > 0) {
        append_str(out, tag);
      }

      append_str(out, s);
      append_html_inline(out, t->child);

      if (tag.len > 0) {
        append_strl(out, "</");
        append_str(out, str_skip(tag, 1));
      }
    }
  }
}

typedef struct Wrap Wrap;
struct Wrap {
  enum BlockType type;
  str open;
  str close;
  str open_line;
  str close_line;
};
#define WRAP(t, o, c, ol, cl) (Wrap){t, strl(o), strl(c), strl(ol), strl(cl)}
void append_wrap(Buf *out, Block *b, Wrap w) {
  w.close_line = w.close_line.len == 0? strl("\n") : w.close_line;

  if (b->type == w.type) {
    append_str(out, w.open);

    for (Text *t = b->text; t; ) {
      Text *tn = t->next; 
      t->next = 0;
      append_str(out, w.open_line);
      append_html_inline(out, t);
      append_str(out, w.close_line);
      t = tn;
    }

    append_str(out, w.close);
  }
}

str append_html(Buf *out, Block *b) {
  append_wrap(out, b, WRAP(PARAGRAPH, "<p>\n", "</p>\n", "", ""));
  append_wrap(out, b, WRAP(QUOTE, "<blockquote><p>\n", "</p></blockquote>\n", "", ""));
  append_wrap(out, b, WRAP(ORD_LIST, "<ol>\n", "</ol>\n", "<li>", "</li>\n"));
  append_wrap(out, b, WRAP(UN_LIST, "<ul>\n", "</ul>\n", "<li>", "</li>\n"));
  append_wrap(out, b, WRAP(RULE, "<hr>\n", "", "", ""));

  if (b->type == TABLE) {
    append_strl(out, "<table class='");
    append_str(out, b->id);
    append_strl(out, "'>\n");
    append_wrap(out, b, WRAP(TABLE, "", "</table>\n", "<tr>", "</tr>\n"));
  }

  if (b->type == HEADING) {
    u8 n = '0' + b->num;
    append_strl(out, "<h");
    append(out, &n, 1);
    append_strl(out, " id='");
    append_str(out, b->id);
    append_strl(out, "'>");

    append_html_inline(out, b->text);

    append_strl(out, "</h");
    append(out, &n, 1);
    append_strl(out, ">");
  }

  if (b->type == CODE) {
    char id[8];
    if (b->id.len == 0) {
      b->id.len = snprintf(id, sizeof(id), "code%03d", b->num);
      b->id.str = (u8*) id;
    }
    append_strl(out, "<code id='");
    append_str(out, b->id);
    append_strl(out, "'><pre>\n");
    s32 line = 1;
    s32 in_comment = 0;
    #define COMMENT_SPAN "<span class='code-comment'>"
    for (Text *t = b->text; t; t = t->next, line++) {
      char id[32]; 
      s32 id_len = snprintf(id, sizeof(id), "%.*s-%d", (s32)b->id.len, b->id.str, line);
      append_strl(out, "<span id='"); 
      append(out, (u8*) id, id_len);
      append_strl(out, "'><a href='#"); 
      append(out, (u8*) id, id_len);
      append_strl(out, "' aria-hidden='true'></a>"); 

      if (in_comment == 2) {
        append_strl(out, COMMENT_SPAN);
      }

      u8 in_string = 0;
      str s = t->s;
      s32 i = 0;

      bool no_comment = str_startl(b->id, "nc");
      while (s.len > 0) {
        while (i < s.len && (char_is_whitespace(s.str[i]) || char_is_alphanum(s.str[i]))) {
          i++;
        }
        append(out, s.str, i);
        s = str_skip(s, i);
        i = 0;
        if (((!no_comment && str_startl(s, "//")) 
        ||  ( no_comment && str_startl(s, "#"))) && in_comment == 0) {
          in_comment = 1;
          append_strl(out, COMMENT_SPAN);
          i = no_comment? 1 : 2;
        } else if (!no_comment && str_startl(s, "/*")) {
          in_comment = 2;
          append_strl(out, COMMENT_SPAN);
          i = 2;
        } else if (in_comment == 2 && str_startl(s, "*/")) {
          append_strl(out, "*/</span>");
          in_comment = 0;
          s = str_skip(s, 2);
        } else if (s.str[0] == '<') {
          append_strl(out, "&lt;");
          s = str_skip(s, 1);
        } else if (s.str[0] == '>') {
          append_strl(out, "&gt;");
          s = str_skip(s, 1);
        } else if (s.str[0] == '&') {
          append_strl(out, "&amp;");
          s = str_skip(s, 1);
        } else if (s.str[0] == '\'' || s.str[0] == '"') {
          i = 1;
          if (in_comment == 0) {
            if (in_string == 0)  {
              append_strl(out, "<span class='code-string'>");
              in_string = s.str[0];
            } else if (in_string == s.str[0]) {
              append(out, &in_string, 1);
              append_strl(out, "</span>");
              in_string = 0;
              s = str_skip(s, 1);
              i = 0;
            }
          }
        } else {
          i = 1;
        }
      }

      if (in_comment > 0) {
        if (in_comment == 1) in_comment = 0;
        append_strl(out, "</span>");
      }
      append_strl(out, "</span>\n");

    }
    append_strl(out, "</pre></code>\n");
  }

  return (str){ out->buf, out->len };
}

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

str read_file(Arena *a, const char *path) {
  str out; 
  struct stat st;
  int fd = open(path, O_RDONLY);
  if (fstat(fd, &st) == 0) {
    out = str_sized(a, st.st_size);
    out.len = read(fd, out.str, st.st_size);
  }
  close(fd);
  return out;
}

bool write_file(const char *path, u8* data, s64 len) {
  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

  s64 n = 0;
  while (n < len) {
    s64 i = write(fd, data + n, len - n);
    if (i > 0) {
      n += i;
    } else {
      break;
    }
  }
  close(fd);
  return n != len;
}

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);
  Arena a = Arena_alloc((Arena){ .size = MB(32) });

  str header = read_file(&a, "src/header.html");
  str footer = read_file(&a, "src/footer.html");
  str rss_header = read_file(&a, "src/rss-header.xml");
  str rss_style = read_file(&a, "src/rss-style.xsl");

  Buf out = {};
  out.cap = MB(2);
  out.buf = Arena_bytes(&a, out.cap);

  Buf rss = {};
  rss.cap = MB(2);
  rss.buf = Arena_bytes(&a, rss.cap);
  append_str(&rss, rss_header);

  ASSERT(chdir("pages/writing") == 0, "ERR: failed to chdir!");
  ARENA_TEMP(a) {
    DIR *dir = opendir(".");
    str *article = Arena_array(&a, str, 4096);
    s32 articles = 0;
    for (struct dirent* f; (f = readdir(dir)); ) {
      if (f->d_type != DT_REG) continue;
      s32 i = 0;
      for (; i < articles; i++) {
        if (memcmp(article[i].str, f->d_name, 3) <= 0) {
          break;
        }
      }
      for (s32 j = articles; j > i; j--) {
        article[j] = article[j-1];
      }
      article[i] = str_copy(&a, strc(f->d_name));
      articles++;
    }
    closedir(dir);

    for (s32 i = 0; i < articles; i++) ARENA_TEMP(a) {
      out.len = 0;
      append_strl(&out, "<!DOCTYPE html>\n");
      append_str(&out, header);

      str md = read_file(&a, str_cstring(&a, article[i]));
      str name = str_trim(str_skip(article[i], 4), 3);

      str frontmatter = str_cut_sub(&md, strl("---"));
      str title = str_skip_startl(str_cut_char(&frontmatter, '\n'), "title: ");
      str date = str_skip_startl(str_cut_char(&frontmatter, '\n'), "date: ");
      str desc = str_skip_startl(str_cut_char(&frontmatter, '\n'), "desc: ");
      append_strl(&rss, "<item>\n<title>");
      append_str(&rss, title);
      append_strl(&rss, "</title>\n<description>");
      append_str(&rss, desc);
      append_strl(&rss, "</description>\n<link>https://loganforman.com/writing/");
      append_str(&rss, name);
      append_strl(&rss, ".html</link>\n<guid>https://loganforman.com/writing/");
      append_str(&rss, name);
      append_strl(&rss, ".html</guid>\n<pubDate>");
      append_str(&rss, date);
      append_strl(&rss, "</pubDate>\n</item>\n");

      append_strl(&out, "<title> 0A ");
      append_str(&out, title);
      append_strl(&out, "</title>\n<div style='clear: both'>\n<h1>");
      append_str(&out, title);
      append_strl(&out, "</h1>\n<h3>");
      append_str(&out, str_first(date, 16));
      append_strl(&out, "</h3>\n</div>\n");

      Block *first = parse_md(&a, md);

      s32 toc_level = 0; s32 toc_first = 0;
      append_strl(&out, "<ul class='sections'>\n");
      for (Block *b = first; b; b = b->next) {
        if (b->type == HEADING) {
          if (toc_first == 0) {
            toc_level = toc_first = b->num;
          }

          for (; toc_level < b->num; toc_level++)
            append_strl(&out, "<ul class='sections'>\n");
          for (; toc_level > b->num; toc_level--)
            append_strl(&out, "</ul>\n");

          append_strl(&out, "<li><a href='#");
          append_str(&out, b->id);
          append_strl(&out, "'>");
          append_html_inline(&out, b->text);
          append_strl(&out, "</a></li>\n");
        }
      }
      for (; toc_level >= toc_first; toc_level--)
        append_strl(&out, "</ul>\n");

      for (Block *b = first; b; b = b->next) {
        append_html(&out, b);
      }

      append_strl(&out, "<hr><p class='centert'>Feel free to email me any comments about this article: <code>contact@loganforman.com</code></p>" );
      append_str(&out, footer);
      
      char filename[256];
      snprintf(filename, sizeof(filename), "../../docs/writing/%.*s.html", (s32)name.len, name.str);
      ASSERT(!write_file(filename, out.buf, out.len), "ERR: failed to write %s!", filename);
    }
  }

  append_strl(&rss, "</channel>\n</rss>\n");

  write_file("../../docs/rss.xml", rss.buf, rss.len);

  rss.len = 0;
  append_str(&rss, str_cut_char(&rss_style, '$'));
  append_str(&rss, header);
  append_str(&rss, str_cut_char(&rss_style, '$'));
  append_str(&rss, footer);
  append_str(&rss, rss_style);
  write_file("../../docs/rss-style.xsl", rss.buf, rss.len);

  ASSERT(chdir("..") == 0, "ERR: failed to chdir!");
  {
    DIR *dir = opendir(".");
    for (struct dirent* f; (f = readdir(dir)); ) ARENA_TEMP(a) {
      if (f->d_type != DT_REG) continue;
      out.len = 0;

      str md = read_file(&a, f->d_name);
      str name = str_trim(strc(f->d_name), 3);
      Block *first = parse_md(&a, md);

      append_strl(&out, "<!DOCTYPE html>\n");
      append_str(&out, header);
      append_strl(&out, "<title> 0A ");
      append_str(&out, name);
      append_strl(&out, "</title>\n");
      for (Block *b = first; b; b = b->next) {
        append_html(&out, b);
      }
      append_str(&out, footer);

      char filename[256];
      snprintf(filename, sizeof(filename), "../docs/%.*s.html", (s32)name.len, name.str);
      ASSERT(!write_file(filename, out.buf, out.len), "ERR: failed to write %s!", filename);
    }
    closedir(dir);
  }

  return 0;
}

