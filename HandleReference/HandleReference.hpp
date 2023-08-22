#pragma once

#include <algorithm>
#include <memory>


template<typename T>
class Handle;


template <typename T>
class Reference
{
public:
    friend class Handle<T>;

    Reference()
        : _ptr(nullptr), _refs(nullptr)
    { }

    ~Reference()
    {
        --(*_refs);

        if ((*_refs & (~Handle<T>::s_InvalidBit)) == 0) // No references, safe to delete
            delete _refs;
    }

    Reference(const Reference &other)
        :_ptr(other._ptr), _refs(other._refs)
    {
        ++(*_refs);
    }

    Reference(Reference&& other) noexcept
        :_ptr(other._ptr), _refs(other._refs)
    {
        other._ptr = nullptr;
        other._refs = nullptr;
    }

    Reference& operator=(const Reference &other)
    {
        if(this == &other)
            return *this;

        _ptr = other._ptr;
        _refs = other._refs;
        ++(*_refs);
    }

    Reference& operator=(Reference&& other) noexcept
    {
        _ptr = other._ptr;
        _refs = other._refs;

        other._ptr = nullptr;
        other._refs = nullptr;
    }


    /// Test whether the referenced memory still exists
    /// \return True if the memory exists, otherwise false
    bool IsValid() const
    {
        return _refs && (*_refs & Handle<T>::s_InvalidBit) == 0;
    }


    // Access
    /// Access the pointer to the underlying object with validity check
    /// \return Pointer to object or nullptr if it no longer exists
    T* Get() const
    {
        if(!IsValid())
            return nullptr;
        return _ptr;
    }

    T* operator->() const { return Get(); }

    T& operator*() const { return *Get(); }

private:
    Reference(T* obj, size_t* refs) : _ptr(obj), _refs(refs)
    {
        ++(*_refs);
    }

private:
    T* _ptr;
    size_t* _refs;
};


template<typename T>
class Handle
{
public:
    friend class Reference<T>;

    Handle()
        : _ptr(nullptr), _refs(nullptr)
    { }

    ~Handle()
    {
        InvalidateRefs();
    }

    friend void swap(Handle& a, Handle& b)
    {
        std::swap(a._ptr, b._ptr);
        std::swap(a._refs, b._refs);
    }

    Handle(const Handle& other) = delete;

    Handle(Handle&& other) noexcept
        : _ptr(std::move(other._ptr))
    {
        InvalidateRefs();
        _refs = other._refs;
        other._refs = nullptr;
    }

    Handle& operator=(const Handle& other) = delete;

    Handle& operator=(Handle&& other) noexcept
    {
        _ptr = std::move(other._ptr);
        InvalidateRefs();
        _refs = other._refs;

        other._refs = nullptr;

        return *this;
    }


    explicit operator bool() const
    {
        return (bool)_ptr;
    }


    template<typename... Args>
    static Handle Make(Args&&... args)
    {
        return Handle(std::make_unique<T>(std::forward<Args>(args)...), new size_t(0));
    }


    // Movement
    /// Moves the memory ownership by offering the Handle as an rvalue
    /// \return Handle rvalue
    Handle Move() { return Handle(std::move(_ptr)); }

    /// Moves the memory ownership by offering the underlying object as an rvalue
    /// \return Object rvalue
    T MoveObj() { return std::move(*_ptr); }

    void Free()
    {
        this->~Handle<T>();
        new (this)Handle<T>;
    }


    // Access
    T* operator->() const { return Get(); }

    T& operator*() const { return *Get(); }

    Reference<T> Ref()
    {
        return Reference<T>(Get(), _refs);
    }

    static const Handle<T> Emtpy;

private:
    template<typename... Args>
    Handle(std::unique_ptr<T> ptr, size_t* refs)
        : _ptr(std::move(ptr)), _refs(refs)
        { }

    T* Get() const { return _ptr.get(); }

    void InvalidateRefs()
    {
        if(!_refs) // Invalid Handle, do nothing
            return;

        if (*_refs == 0) // No references, safe to delete
            delete _refs;
        else // References exist, set invalid bit
            *_refs |= s_InvalidBit;
    }

private:
    std::unique_ptr<T> _ptr;
    size_t *_refs;

    static constexpr size_t s_InvalidBit = static_cast<size_t>(1) << (sizeof(size_t) * 8 - 1);
};
