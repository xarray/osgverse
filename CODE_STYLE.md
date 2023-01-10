# osgVerse Code style and Formatting
Copied and modified from Google Filament code style.
Remember: break the rules if it makes sense, e.g. it improves readability substantially.

## Formatting
- 4 spaces indent
- 8 spaces continuation indent
- 100 columns
- `{` at the beginning of new line
- spaces around operators, and after `;` in loop conditions
- class access modifiers are not indented
- third party source code may use the formatting of their own

```
for (int i = 0; i < max; ++i)
{
}

class Foo
{
public:
protected:
private:
};

```

## Naming Conventions

### Files
- headers use the `.h` extension
- implementation files use the `.cpp` extension
- included files use the `.inline` extension
- **no spaces** in file names
- use `#include < >` for all public (exported) headers
- use `#include " "` for private headers
- tests reside under the `tests` folder
- public headers of a `foo` library must live in a folder named `foo`

```
include/foo/FooBar.h
src/FooBar.cpp
src/data.inc

#include <foo/FooBar.h>
#include "FooBarPrivate.h"
```

### Code
- Everything is camel-cased except constants
- `constants` are upper-cased and don't have a prefix
- `global` variables prefix with `g_`
- `static` variables and class attributes prefix with `s_`
- `private` and `protected` class attributes prefixed with `_`
- `public` class attributes prefix with `_` or not prefixed
- class names are upper camel-cased
- all variables, class attributes and methods are lower camel-cased

```
extern int g_globalWarming;
const int FOO_COUNT = 10;

class FooBar
{
public:
    void methodName(int arg1, bool arg2);
    int sizeInBytes;
    
private:
    int _attributeName;
    static int s_globalAttribute;
    enum { ONE, TWO, THREE };
};
```

### Misc
- When using STL, always include the `std::` qualifier to disambiguate it from others.
