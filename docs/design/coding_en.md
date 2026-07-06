### C++ Coding Style



### Header File

#### Name and Order of Includes

The order is:

Related header, C library, C++ library, other libraries' `.h`, your project's `.h`.

Do not use `.` or `..` symbols.

For example, a project's include header should look like this:

```c++
#include "foo/server/fooserver.h"

#include <sys/types.h>
#include <unistd.h>

#include <hash_map>
#include <vector>

#include "base/basictypes.h"
#include "base/commandlineflags.h"
#include "foo/server/bar.h"

```



### Class

#### Doing Work in Constructors

Constructors cannot call virtual functions, because at construction time the object has not been fully created yet, so calling a virtual function would be wrong.



#### Inheritance

Use `override` to indicate that a function is an override of a virtual function. This way you don't need to verify whether the function is an overload while reading code. If you `override` a function that doesn't exist in the parent class, it will cause a compile error directly.

When using `struct`, only use it for structures that only define data and contain no methods other than simple constructors or `init` functions.

Using inheritance can effectively reduce code volume, and since inheritance is a compile-time feature, the compiler can identify these errors. Interface inheritance (defining pure virtual functions) can identify at compile time whether a derived class implements all the interfaces.

However, since inheritance scatters a class's code across multiple files, it increases the difficulty of reading code, and the parent class defines its own member variables, which makes access inconvenient.

Therefore, you must always distinguish between is-a and has-a relationships. Only use inheritance when a is a kind of b. **Otherwise, prefer composition**, i.e., b contains an a member variable.

### Function

#### Parameter Ordering

Function parameter order: input first, then output.

Try to keep a function within 40 lines.

#### Reference Arguments

All variables passed by reference must be marked with `const`, i.e., `const type &in`.

As much as possible, input arguments should use value or const reference (of course, if the variable is a pointer, pass it as a pointer), and output arguments should use pointers.

Also, if a variable needs to accept `NULL` as input, you might use `const T*`.

#### Function Overloading

Avoid function overloading as much as possible, because function overloading increases the complexity of C++. Especially with inheritance, when a subclass only implements one of the parent class's overloaded functions, the code complexity becomes even harder to manage because it's unclear which overload is being overridden. Therefore:

Avoid function overloading as much as possible. When a function needs to handle different variable types, write them like `AppendString()`, `AppendInt()`, etc.

#### Default Values

Allowed to use default values in non-virtual functions.

### Scoping

#### Nonmember, Static Member, Global Functions

If a function is unrelated to the contents of a class's objects:

There are two options: define it as a class static member function, or a nonmember function. How to choose?

If the function is strongly related to the object — for example, creating an instance of that class or operating on a static member of the class — declare it as a class static member function.

Otherwise, declare it as a nonmember function and isolate it using a namespace.

If a function is only used in a specific `.cc` file, you can place it in an unnamed namespace or use `static` to declare it as `static int foo()`.



### Other

#### On the Use of Exceptions

* Pros:
  * Exceptions can catch deeper-level errors. For example, in `a()->b()->c()->d()`, an exception thrown in `d()` can be caught directly in `a()`.
  * In C++ constructors, we cannot know whether construction succeeded.
* Cons
  * ​

#### On Return Values

1. Called inside a function.



#### Brace Initializer List

In C++11, you can initialize a list directly with `{}`, which was not possible before C++11, for example:

```c++
int main()
{
  std::vector<int> v{1, 2, 3};
  std::map<int, int> mp{{1, 2}, {1, 3}, {1, 4}};
  return 0;
}
```

#### sizeof

When using `sizeof`, prefer `sizeof(varname)` over `sizeof(type)`, because `varname` may be updated at any time. If the variable `varname` is assigned to another object, the size could change.

Be mindful of alignment when using `sizeof`.

####  Run-Time Type Information (RTTI)

C++ allows using `typeid` and `dynamic_cast` at runtime to check a variable's type. `dynamic_cast` performs type checking during conversion, allowing only parent class pointers to point to child classes, not the reverse.

However, code that uses RTTI can generally be rewritten in other ways, and RTTI is not very efficient. Therefore, prefer using virtual methods or the Visitor pattern instead.

#### Cast

Prefer C++ casts (`static_cast`, `const_cast`, `reinterpret_cast`) over C-style casts.

#### Stream

If you want to print internal details of an object for debugging, the most common approach is to provide a `DebugString()` method.

Do not use streams for external user I/O; stream performance is not great.

#### Friend

`Friend` class and function usage is allowed.

`Friend` class breaks class encapsulation by allowing an external class direct access to private members of the current class. A common use case is that `FooBuilder` should be able to access private members of `Foo`. Without `Friend`, you'd either have to make all of Foo's members public or add getters/setters for all member variables — both are inconvenient.

A `Friend` class only allows one specific class to access the private members. This is still better encapsulation than making all member variables public.

Since `Friend` classes need to see the private variables of `Foo`, `Friend` classes are often placed in the same header file.

#### Use of const

Use `const` wherever possible.

#### Integer Type

Use `int32_t`, `int64_t`, etc. from `<stdint.h>` instead of `short`, `long`, `long long`, because the sizes of `short`, `long`, etc. depend on the compiler and platform.

#### 0 and nullptr/NULL

Use 0 for integers, 0.0 for reals, nullptr (or NULL) for pointers, and '\0' for chars.

In projects supporting C++11, prefer `nullptr`.

### Comments

#### TODO Comment

When writing a TODO comment, remember to include who wrote it.

// TODO(kl@gmail.com): Use a "*" here for concatenation operator.


### Summary

Finally, run `cpplint.py` and try to eliminate errors at severity level 4 and above.
