# libCello Macro Wizardry Report

`libCello` is a masterclass in C preprocessor abuse (in the best way possible). It pushes the boundaries of the language to provide features like high-level syntax, object-oriented programming, and garbage collection.

## 1. The `main` Redefinition (GC Hijacking)

Cello uses a clever trick to ensure the Garbage Collector (GC) is initialized before any user code runs.

```c
#define main(...) \
  main(int argc, char** argv) { \
    var bottom = NULL; \
    new_raw(GC, $R(&bottom)); \
    atexit(Cello_Exit); \
    return Cello_Main(argc, argv); \
  }; \
  int Cello_Main(__VA_ARGS__)
```

- **How it works:** When you write `int main(int argc, char** argv) { ... }` in your code, the macro expands it into a wrapper `main` function.
- **The Wrapper:** It records the "bottom" of the stack (essential for conservative GC scanning), initializes the `GC` object, registers a cleanup function via `atexit`, and then calls your code (now renamed to `Cello_Main`).
- **Result:** Transparent GC setup without the user ever calling `gc_init()`.

## 2. Prefixed Header (The "Fat Pointer" Trick)

Most Cello objects are just regular C pointers, but they hide metadata in the memory addresses immediately _preceding_ the pointer.

```c
struct Header {
  var type;
#if CELLO_ALLOC_CHECK == 1
  var alloc;
#endif
#if CELLO_MAGIC_CHECK == 1
  var magic;
#endif
};
```

Cello uses macros like `alloc_stack` to allocate space for both the header and the data in one go:

```c
#define alloc_stack(T) ((struct T*)header_init( \
  (char[sizeof(struct Header) + sizeof(struct T)]){0}, T, AllocStack))
```

- **Compound Literals:** It uses a C99 compound literal `(char[...]){0}` to allocate space on the stack.
- **Offsetting:** `header_init` populates the header at the start of that memory block and returns a pointer to the address _after_ the header.
- **Polymorphism:** This allows any function to take a `var` (a `void*`), subtract `sizeof(struct Header)`, and suddenly know the object's type, magic number, and allocation status.

## 3. The `$` Macro (Literal-like Stack Objects)

The `$` macro is the primary way users create objects on the stack.

```c
#define $(T, ...) ((struct T*)memcpy( \
  alloc_stack(T), &((struct T){__VA_ARGS__}), sizeof(struct T)))
```

- **Seamless Initialization:** It combines `alloc_stack` (which reserves space and sets up the header) with a standard C struct initializer.
- **`memcpy` return value:** Since `memcpy` returns the destination pointer, the whole macro evaluates to the pointer to the new Cello object.
- **Shorthands:** Macros like `$I(X)` for integers or `$S(X)` for strings make C code look almost like a high-level language: `var x = $I(10);`.

## 4. `foreach` and `with` (Custom Control Flow)

Cello implements a `foreach` loop that looks like Python or Java but is pure C.

```c
#define foreach(...) foreach_xp(foreach_in, (__VA_ARGS__))
#define foreach_xp(X, A) X A
#define foreach_in(X, S) for(var \
  __##X = (S), \
  __Iter##X = instance(__##X, Iter), \
  X = ((struct Iter*)(__Iter##X))->iter_init(__##X); \
  X isnt Terminal; \
  X = ((struct Iter*)(__Iter##X))->iter_next(__##X, X))
```

- **Expansion Trick:** `foreach_xp` (Expansion) is a trick used to ensure arguments are expanded correctly before being passed to `foreach_in`.
- **Iteration State:** It creates hidden variables (like `__Iter##X`) to store the iterator instance, then uses the standard `for` loop structure to call `iter_init` and `iter_next`.

## 5. Exception Handling (`try` / `catch` / `throw`)

Cello provides a full exception system using `setjmp` and `longjmp`.

```c
#define try { jmp_buf __env; exception_try(&__env); if (!setjmp(__env))
#define catch(...) catch_xp(catch_in, (__VA_ARGS__))
#define catch_in(X, ...) else { exception_try_fail(); } exception_try_end(); } \
  for (var X = exception_catch(tuple(__VA_ARGS__)); \
    X isnt NULL; X = NULL)
```

- **Control Flow Hijacking:** The `try` macro opens a block and uses `if (!setjmp(...))`. If a `throw` occurs later, `longjmp` returns to this point, but `setjmp` returns non-zero, triggering the `else` (the `catch` block).
- **The `for` Loop in `catch`:** Using a `for` loop inside the `catch` macro allows the user to declare a local variable `X` (the exception object) that is scoped only to that catch block.

## 6. Variadic Tuple Magic

Creating a `tuple` from a variadic list of arguments in C is notoriously difficult. Cello uses a "Terminal" marker trick.

```c
#define tuple(...) tuple_xp(tuple_in, (_, ##__VA_ARGS__, Terminal))
#define tuple_xp(X, A) X A
#define tuple_in(_, ...) $(Tuple, (var[]){ __VA_ARGS__ })
```

- **`##__VA_ARGS__`**: This is a GCC/Clang extension that handles the case where no arguments are passed (it removes the preceding comma).
- **Array Casting:** It turns the arguments into an anonymous array of `var` on the stack, which is then passed to the `Tuple` constructor.

## 7. Syntax Sugar

Simple but effective "Human-friendly" aliases:

```c
#define is ==
#define isnt !=
#define not !
#define and &&
#define or ||
#define in ,
```

This allows code like `if (x isnt NULL and y is $I(10))`. Note the `in` alias, which is used specifically to make the `foreach` macro read more naturally: `foreach (item in items)`.

## 8. The `CelloObject` Macro (Self-Assembling Type Metadata)

This is how `libCello` defines the "Type" objects themselves (like `Array` or `Int`) without needing a separate compilation step for metadata.

```c
#define CelloObject(T, S, ...) (var)((char*)((var[]){ NULL, \
  CELLO_ALLOC_HEADER       \
  CELLO_MAGIC_HEADER       \
  CELLO_CACHE_HEADER       \
  NULL, "__Name",     #T,  \
  NULL, "__Size", (var)S,  \
  ##__VA_ARGS__,           \
  NULL, NULL, NULL}) +     \
  sizeof(struct Header))
```

- **Anonymous Arrays:** It creates an anonymous array of `var` on the fly.
- **In-place Casting:** It casts that array to a `char*`, adds the size of the Cello `Header`, and casts it back to `var`.
- **The Result:** You get a pointer that points _after_ the metadata, just like a heap-allocated object, but the data is actually stored in the program's data segment (if global) or stack.

## 9. Dynamic Dispatch (`method`)

This is the heart of Cello's polymorphism. It's what allows `len(x)` to work whether `x` is an `Array`, a `Table`, or a `String`.

```c
#define method(X, C, M, ...) \
  ((struct C*)method_at_offset(X, C, \
  offsetof(struct C, M), #M))->M(X, ##__VA_ARGS__)
```

- **`offsetof` trickery:** It uses the standard C `offsetof` macro to find exactly where a specific function pointer (the "method") lives within a Class structure (like `struct Len` or `struct Iter`).
- **Dynamic Lookup:** `method_at_offset` finds the class implementation for the object's type, and the macro then calls the function pointer discovered at that offset.

## 10. Context Managers (`with`)

Cello brings Python's `with` statement to C using a `for` loop hack.

```c
#define with(...) with_xp(with_in, (__VA_ARGS__))
#define with_in(X, S) for(var X = start_in(S); X isnt NULL; X = stop_in(X))
```

- **How it works:** `start_in` (usually calls a `lock` or opens a file) returns the object itself. The loop runs once. When the loop finishes (or is broken), `stop_in` is called (which unlocks or closes).
- **Usage:** `with (mut in mutex) { ... }` provides automatic resource management in pure C.

## 11. Compile-Time Argument Counting (`zip`)

The `zip` macro needs to know how many arguments you passed so it can allocate a result tuple of the correct size.

```c
#define zip(...) zip_stack( \
  $(Zip, tuple(__VA_ARGS__), \
  $(Tuple, (var[(sizeof((var[]){__VA_ARGS__})/sizeof(var))+1]){0})))
```

- **`sizeof` on Array Literals:** It uses `sizeof((var[]){__VA_ARGS__}) / sizeof(var)` to count the number of arguments at compile time.
- **VLA/Compound Literal:** It uses that count to create a stack-allocated array of exactly the right size to hold the results of the zip operation.
