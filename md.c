#define LF_IMPL
#include "lf.h"

enum TextStyle  {
  NONE,
  BOLD, ITALIC, STRUCK, CODE_INLINE,
  LINK, IMAGE, EXPLAIN,
  TABLE_CELL,
  TEXT_STYLES,
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
  SPECIAL
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

    if (str_has_prefix(line, strl("```"))) { // special case for code blocks
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

    } else if (str_has_prefix(line, strl("@{"))) {
      line = str_skip(line, 2);
      b = push_block(a, b, 0, 0);
      b->id = str_cut_delims(&line, strl(",}"));
      b = push_block(a, b, SPECIAL, &line);
    } else if (str_has_prefix(line, strl("---"))) {
      b = push_block(a, b, RULE, 0);

    } else if (str_has_prefix(line, strl("- "))) { 
      // TODO(lf) UN_LIST and ORD_LIST dont support nested lists
      line = str_skip(line, 2);
      b = push_block(a, b, UN_LIST, &line);

    } else if (char_is_num(line.str[0]) && line.str[1] == '.' && line.str[2] == ' ') {
      line = str_skip(line, 3);
      b = push_block(a, b, ORD_LIST, &line);

    } else if (str_has_prefix(line, strl("!|"))) {
      line = str_skip(line, 2);

      if (b->type != TABLE) {
        str id = str_cut_char(&line, '|');
        b = push_block(a, b, TABLE, &line);
        b->id = id;
      } else {
        b = push_block(a, b, TABLE, &line);
      }

    } else if (str_has_prefix(line, strl("> "))) {
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
    if (match_prefix(&s, "\\", &tok_len)) { s = str_skip(s, 1); } 
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

  p->s = str_first(p->s, (s.str - p->s.str) - tok_len);

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
      // TODO ugly that I am wrapping more here
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
  s32 amount = b->err? 0 : MIN(avail, len);
  for (s32 i = 0; i < amount; i++) {
    b->buf[b->len+i] = data[i];
  }
  b->len += amount;
  b->err |= amount < len;
}
#define append_strl(b, sl) append(b, (u8*)sl, sizeof(sl"")-1)
void append_str(Buf *b, str s) { append(b, s.str, s.len); }

void render_inline(Buf *out, Text *t) {
  const str tags[TEXT_STYLES] = {
  [BOLD] = strl("<b>"), 
  [ITALIC] = strl("<em>"), 
  [STRUCK] = strl("<s>"),
  [CODE_INLINE] = strl("<code>"),
  [TABLE_CELL] = strl("<td>"),
  // LINK, IMAGE, EXPLAIN, // not covered TODO handle special cases
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
      append_strl(out, "<abbr title='");
      append_str(out, str_cut_char(&s, ','));
      append_strl(out, "'>");
      append_str(out, s);
      append_strl(out, "</abbr>");
    } else if (t->type == IMAGE) {
      if (str_has_suffix(s, strl(".mp4"))) {
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
      render_inline(out, t->child);

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
void render_wrap(Buf *out, Block *b, Wrap w) {
  w.close_line = w.close_line.len == 0? strl("\n") : w.close_line;

  if (b->type == w.type) {
    append_str(out, w.open);

    for (Text *t = b->text; t; ) {
      Text *tn = t->next; 
      t->next = 0;
      append_str(out, w.open_line);
      render_inline(out, t);
      append_str(out, w.close_line);
      t = tn;
    }

    append_str(out, w.close);
  }
}

str append_html(Buf *out, Block *b) {
  render_wrap(out, b, WRAP(PARAGRAPH, "<p>\n", "</p>\n", "", ""));
  render_wrap(out, b, WRAP(QUOTE, "<blockquote><p>\n", "</p></blockquote>\n", "", ""));
  render_wrap(out, b, WRAP(ORD_LIST, "<ol>\n", "</ol>\n", "<li>", "</li>\n"));
  render_wrap(out, b, WRAP(UN_LIST, "<ul>\n", "</ul>\n", "<li>", "</li>\n"));
  render_wrap(out, b, WRAP(RULE, "<hr>\n", "", "", ""));

  if (b->type == TABLE) {
    append_strl(out, "<table class=");
    append_str(out, b->id);
    append_strl(out, ">\n");
    render_wrap(out, b, WRAP(TABLE, "", "</table>\n", "<tr>", "</tr>\n"));
  }

  if (b->type == HEADING) {
    u8 n = '0' + b->num;
    append_strl(out, "<h");
    append(out, &n, 1);
    append_strl(out, " id='");
    append_str(out, b->id);
    append_strl(out, "'>");

    render_inline(out, b->text);

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
    #define COMMENT_SPAN "<span style='color: var(--comment);'>"
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

      char in_string = 0;
      str s = t->s;
      s32 i = 0;
      while (s.len > 0) {
        while (i < s.len && (char_is_whitespace(s.str[i]) || char_is_alphanum(s.str[i]))) {
          i++;
        }
        append(out, s.str, i);
        s = str_skip(s, i);
        i = 0;
        if (str_has_prefix(s, strl("//"))) {
          in_comment = 1;
          append_strl(out, COMMENT_SPAN);
          i = 2;
        } else if (str_has_prefix(s, strl("/*"))) {
          in_comment = 2;
          append_strl(out, COMMENT_SPAN);
          i = 2;
        } else if (in_comment == 2 && str_has_prefix(s, strl("*/"))) {
          in_comment = 0;
          i = 2;
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
          if (in_comment == 0) {
            if (in_string == 0)  {
              append_strl(out, "<span style='color: var(--red);'>");
            } else if (in_string == s.str[0]) {
              append_strl(out, "</span>");
            }
          }
          i = 1;
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

str header;
str footer;

Arena a;
Buf out;
Buf rss;

void make_page(Block *first) {
  // append_str(&out, header);
  for (Block *b = first; b; b = b->next) {
    if (b->type == SPECIAL) {

    } else {
      append_html(&out, b);
    }
  }
  // append_str(&out, footer);

}


int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);
  a = Arena_new((Arena){ .size = MB(32) });

  header = read_file(&a, "header.html");
  footer = read_file(&a, "footer.html");
  str rss_header = read_file(&a, "rss-header.xml");

  out.cap = MB(2);
  out.buf = Arena_take(&a, out.cap);

  rss.cap = MB(2);
  rss.buf = Arena_take(&a, rss.cap);
  append_str(&rss, rss_header);

  chdir("pages/writing");
  {
    DIR *dir = opendir(".");
    for (struct dirent* f; (f = readdir(dir)); out.len = 0) 
      ARENA_TEMP(a) {
      if (f->d_type != DT_REG) continue;
      
      str name = strc(f->d_name);
      str md = read_file(&a, f->d_name);
      Block *first = parse_md(&a, md);
      make_page(first);

      printf("\n\n%.*s\n", (s32) name.len, (char*) name.str);
      printf("%.*s\n", (s32) out.len, (char*) out.buf);
    }
    closedir(dir);
  }

  chdir("..");
  {
    DIR *dir = opendir(".");
    for (struct dirent* f; (f = readdir(dir)); out.len = 0) 
      ARENA_TEMP(a) {
      if (f->d_type != DT_REG) continue;

      str name = strc(f->d_name);
      str md = read_file(&a, f->d_name);
      Block *first = parse_md(&a, md);
      make_page(first);

      printf("\n\n%.*s\n", (s32) name.len, (char*) name.str);
      printf("%.*s\n", (s32) out.len, (char*) out.buf);
    }
    closedir(dir);
  }

  return 0;
}

