#define LF_IMPL
#include "lf.h"

enum TextType  {
  TEXT = 1,
  BOLD, ITALIC, STRUCK, CODE_INLINE,
  LINK, IMAGE, EXPLAIN,
  TABLE_CELL,
  LIST_ITEM, CODE_BLOCK,
};

typedef struct Text Text;
struct Text {
  Text *next;
  Text *child;
  enum TextType type;
  str s;
};

enum BlockType {
  NONE,
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

  s32 code_block_number = 0;
  while (input.len > 0) {
    str raw = str_cut_char(&input, '\n');
    str line = str_skip_whitespace(raw);

    if (str_has_prefix(line, strl("```"))) { // special case for code blocks
      if (b->type != CODE) {
        b = push_block(a, b, CODE, 0);
        b->id = str_trim_whitespace(str_skip(line, 3));
        b->num = code_block_number++;
      } else {
        b = push_block(a, b, NONE, 0);
      }

    } else if (b->type == CODE) {
      b = push_block(a, b, CODE, &line);

    } else if (line.len == 0) {
      b = push_block(a, b, NONE, 0);

    } else if (str_has_prefix(line, strl("@{"))) {
      line = str_skip(line, 2);
      b = push_block(a, b, NONE, 0);
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
      b = push_block(a, b, NONE, 0);
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

Block* push_block(Arena *a, Block *b, enum BlockType type, str *line) {
  if (b->type != type && b->type != NONE) {
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

    // if (b->type != CODE) {
      // parse_inline(a, b->text)    
    // }
  }

  return b;
}

#include <stdio.h>
#include <fcntl.h>     // open(), O_RDONLY
#include <unistd.h>    // read(), close(), ssize_t (sometimes needed)
#include <sys/stat.h>  // struct stat, fstat()

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

int main(int argc, char *argv[]) {
  Arena a = Arena_new((Arena){ .size = MB(32) });
  str test = read_file(&a, argv[1]);
  Block *first = parse_md(&a, test);
  for (Block *b = first; b; b = b->next) {
    if (b->id.len > 0) {
      printf("#%.*s ", (int) b->id.len, b->id.str);
    }
    for (Text *t = b->text; t; t = t->next) {
      switch (b->type) {
      case NONE: printf("NONE"); break;
      case PARAGRAPH: printf("PARAGRAPH"); break;
      case HEADING: printf("HEADING"); break;
      case RULE: printf("RULE"); break;
      case CODE: printf("CODE"); break;
      case TABLE: printf("TABLE"); break;
      case QUOTE: printf("QUOTE"); break;
      case ORD_LIST: printf("ORD_LIST"); break;
      case UN_LIST: printf("UN_LIST"); break;
      case SPECIAL: printf("SPECIAL"); break;
      }
      printf(" %.*s\n", (int) t->s.len, t->s.str);
    }
    printf("\n");
  }
}