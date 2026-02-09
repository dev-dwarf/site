title: Remaking My Static Site Generator
date: Sun, 08 Feb 2026 21:00:00 MST
desc: ~3 years on from my first post, I redid the SSG with new skills.
---

##introd Introduction
I started this blog with a post: @(/writing/making-a-ssg1.html "Making A Static Site Generator"). In addition to serving 
as a portfolio website when I was graduating college, it was also an opportunity
to test out a new style of programming for me, where instead of using C's archaic
string handling facilities I used my own platform layer/base library. The SSG was 
also my first significant project using @(https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator arena allocation) 
instead of `malloc`/`free`.

Just over 3 years later, I decided to revisit this project for a few reasons:
- I've refined what I want out of a base library now that I have more experience writing programs that way. Just like the first time I made a base library, this is a simple project to test that code out with.
- The primary OS I code on has changed from Windows to Linux, and the old site generator was only made to compile on Windows. This has discouraged me from making posts.
- Finally, the site generator itself had some weird quirks that annoy me now.

The result is a smaller more sensible site generator that reflects how I would 
make this kind of a project at my current skill level.

---

##lf New Base library
When I made my first base library, @(https://github.com/dev-dwarf/lcf lcf), I was 
very new to the concept and for the most part directly copied the structure from
rjf's base layer. Since then, I've done more programming this way for fun and 
professionally, and I've seen more examples of similar projects, so I developed more opinions on what I prefer. My new library is @(https://github.com/dev-dwarf/lf lf), and has the following differences:

###sh Single Header
Instead of a tree of `.c` and ``.h` files, for now I'm sticking to a 
single header, `lf.h`. This encourages me to keep this core functionality lean
and avoid bloating it with functions I actually only need for one project, 
which was a common issue for me before.

For now I've reduced the functionality to just typedefs, macros, arena allocation,
and strings. I could still see myself adding more functionality than the current
bare-bones core, but its nice to have a blank-*ish* slate after accumulating cruft
for 3 years in `lcf`.

###arena Arenas
I've changed my `Arena` implementation to support both Virtual-Memory style Arena's that commit over time, as well as creating an arena from an existing fixed size buffer. I've found it very convenient, especially on embedded devices, to just 
quickly make an arena from a fixed size piece of memory without having to go through
any sort of dynamic allocation facility.
```
typedef struct Arena {
  u8 *buf;
  uptr pos;
  uptr size;
  uptr commit_pos; // for fixed size, should be uptr_MAX
  sptr commit_size;
} Arena;

// Create arenas
#define Arena_buffer(buf) Arena_fixed(buf, sizeof(buf))
Arena Arena_fixed(u8* buf, uptr size);
Arena Arena_alloc(Arena params);
void Arena_destroy(Arena *a); // NOTE(lf): Not needed at program exit
```

###str Strings
I've changed the type of my `str` struct to `u8 *`, an unsigned byte. This is
inconvenient for inter-operating with traditional C functions, but makes the 
str type much more useful for general non-text data, which is now a common use 
case for me. I also define the `len` field with `sptr` instead of `s64` for the length,
which I've come to appreciate after using these types on 32-bit devices.
```
typedef struct str {
  u8 *str;
  sptr len;
} str;
```

I have removed the many `str` functions I had but never used, and now have a more 
minimal set which I have renamed for consistency. 
I've also completely remove `StrList`s, which were previously used to build up
larger strings over multiple calls using linked lists. I still appreciate the 
@(https://www.rfleury.com/p/in-defense-of-linked-lists power of linked lists), but 
for strings specifically I have not found them that useful. For this project I used
@(https://nullprogram.com/blog/2023/02/13/ buffered, formatted output), which 
for now lives in my SSG code but may eventually be moved into my
base layer.

---

##yotld Year of the Linux Desktop
Microsoft seems dead set on destroying Windows with Windows 11, which I have been lucky enough to avoid using so far. For the last few years I have primarily been programming on Linux for work anyways, so I have much more experience with that 
environment than in the past. The lack of high-quality debuggers is less of a problem 
on Linux with @(https://github.com/al13n321/nnd nnd) and a forthcoming
port of @(https://github.com/EpicGamesExt/raddebugger raddebugger). Overall Linux 
seems to be a more viable platform for serious computing at this point.

Because of this, I've switched my main development machine to Linux and I am treating that as the primary target for my base layer and the SSG.
Specifically I have switched to Nix-OS, and so I have also adopted Nix as a general
package management system and used it for my base layer and this project. I still 
have mixed feelings about Nix; I would not consider myself an expert in the language
or the finer details of the package manager, and I probably wouldn't have learned it 
without needing to for work. But I can acknowledge that he data model is powerful and
I have seen real benefits in the level of control and reproducability it offers.
Surprisingly, NixOS had the smoothest install of any Linux distro I've tried, with 
all the functionality on my machine working out of the box. I also appreciate that
I can choose when to update without any annoying popups, and if something goes wrong,
I can easily roll back to a working version of the OS.

I have a flake for both my base layer and the SSG. The base layer flake defines an overlay with a convenience function to build a ?(Single Translation Unit,STU) project:
```nc-flake
{
  description = "lf c library";

  outputs = { self, nixpkgs }: {
    overlay = final: prev: rec {
      # Just copies headers into nix store for easy #include
      lf = prev.stdenv.mkDerivation rec {
        name = "lf";
        src = ./.;
        dontBuild = true;
        allowSubstitutes = false;
        installPhase = '' mkdir -p $out/include && cp -r $src/lf.h $out/include/'';
      };

      # Standard derivation template for building Single Translation Unit programs
      mkSTU = let 
        mk = {name, src
        , main ? name # default main src file is $name.cpp or $name.c
        , buildInputs ? []
        , flags ? ""
        , debug ? false
        , cpp ? false
        , executable ? true
        , installPhase ? ''
          runHook preInstall;
          install -Dt $out/bin ${name};
          runHook postInstall;
        ''
        , ...}@args:
          prev.stdenv.mkDerivation (args // {
            inherit name src installPhase;
            buildInputs = [ lf ] ++ buildInputs;
            doCheck = !debug;
            dontStrip = debug;
            allowSubstitutes = false;
            buildPhase = 
            ''${if cpp then "$CXX" else "$CC"} \
              ${if executable then "-o ${name}" else "-o {name}.o -c"} \
              ${src}/${main}.${if cpp then "cpp" else "c"} \
              ${flags} ${if debug then "-g -O0" else "-O2"}
            '';
          });
        in args: (mk args) // { debug = mk (args // { debug = true; }); };
    };
  };
}
```
This has really helped me bridge the gap between the Nix world and a more handmade
style of programming. I appreciate Nixpkg's ability to package even the most convoluted
build processes into a simple command I can call, but for my own projects I'd rather
have a simpler and faster build. Even if a project ends up needing more than one
translation unit, I've found it better to be intentional about how I'm splitting things
up than to just throw it all at something like make. `mkSTU` also adds a debug build
for any package as simple as calling `nix build .#package.debug`, which is really nice.
For some good reasons, and some less good ones, Nix usually removes all debug info, so 
its nice to have debug builds built-in without having to remember how to get nix to 
keep it each time. I also add `allowSubstitutes = false;` to avoid querying online
caches for all little programs. 

I'm excited to use flakes for my projects with this system. A common problem I had before was that I would make changes to my base library while working on a project,
and unintentionally break an older project. With the way im using flakes, each project
will be locked to the specific version of my base library it was built with until
I explicitly update, which should ensure I always have a working build of each project.

---

##quirks General Cleanup
I've combined the 4 `.c` and `.h` files I had before into just one, `site.c`. The 
parser/renderer for the markdown-like language I write pages in is not worth attempting
to reuse outside of this site it was made for, so there's not much point in having a
separate file.

The old version did a lot of song and dance with glob patterns and paths to find all 
the `.md` files to compile and put the output `.html` files in the right place. I don't
think this was worth it, so in the new version I just use `chdir()` and `opendir(".")` 
to avoid needing to build up any paths as strings. This isn't "scaleable", and doesn't 
need to be; I have exactly two folders of `.md` files to deal with.

The `@{SPECIAL}` block mechanism I made a @(https://loganforman.com/writing/making-a-ssg2.html whole post on) also does not seem needed. In practice I did
not use it for much, so its just simpler to handle the special cases for giving posts
a title and publish date and building a feed manually. While doing that I went ahead 
and combined my post index page and rss feed by creating a `.xsl` file that styles my 
rss feed to look exactly how the post index looked before.

I've also removed all the big multiline strings from the C file, moving them to separate, external files. This gets me better syntax highlighting of those files
in my editor, and lets me avoid C's pretty weak multi-line string support.

---

##conc Conclusion
Overall these changes have gotten the SSG down from 889 to 694 LOC, over 20% smaller. 
If I later decide to move the buffered output into my base layer, that number would go 
down a little bit more. It was fun to revisit this project; I've learned a lot since my 
first attempt. I should be able to make my yearly posts now that I can do updates from 
Linux. Here's hoping the next one is not about making the website itself; so far I'm 1
for 4 on that.


