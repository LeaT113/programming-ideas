# Handle and Reference
Expanding on the idea of `unique_ptr` and trackable memory ownership, I present `Handle` and `Reference`.

## Usage
`Handle` allows you to manage memory like `unique_ptr`, having a `Handle` directly **own** memory, knowing it will be freed once it gets destroyed. It can also **move** ownership to another `Handle`.

`Reference` takes the place of raw pointers to the object. It guarantees memory safety by checking the validity of the memory it references. You can check by calling `Reference<T>::IsValid()` which will return false if the `Handle` it references has been destroyed and the memory freed.

## Inner workings
A `Handle` is a thin wrapper around `unique_ptr` but alongside the object, it also heap allocates a `size_t` representing **reference count**. If no references of the `Handle` currently exist, it frees the reference count on destruction.

A `Reference` stores a raw data pointer and a pointer to the reference count, incrementing it on construction and decrementing on destruction.

When a `Handle` gets destroyed while some `Reference`s to it exist, it will set the highest bit of the `size_t` to 1, which indicates the data pointer is invalid. `Reference` can check for this bit, not allowing access to the data pointer if it's been set. Once the last `Reference` to the same, dead `Handle` gets destroyed, it frees the reference count.
