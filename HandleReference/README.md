# Handle and Reference
Expanding on the idea of `unique_ptr` and trackable memory ownership, I present `Handle`s and `Reference`s.

## Usage
`Handle`s allow you to manage memory like `unique_ptr`, having a `Handle` directly _own_ memory, knowing it will be freed once it gets destroyed. They can also **move** ownership to another `Handle`.
`References` take the place of raw pointers to the underlying memory. They guarantee memory safety by checking the validity of the memory they reference. You can check by calling `Reference<T>::IsValid()` which will return false if the `Handle` they reference has been destroyed and the memory freed.

## Inner workings
A `Handle` is a thin wrapper around `unique_ptr` but alongside the object, it also heap allocates a `size_t` representing reference count. If no references of the `Handle` were created, it frees this memory on destruction.

A `Reference` will store a raw data pointer and a pointer to the reference count, incrementing it on construction and decrementing on destruction.

When a `Handle` gets destroyed while the reference count is not 0, it will set the highest bit of the `size_t` to 1, which indicates it's been destroyed. `Reference`s can check for this bit, not allowing access to the data pointer if it's been set. Once the last `Reference` to the same, dead `Handle` gets destroyed, it frees the reference count.
