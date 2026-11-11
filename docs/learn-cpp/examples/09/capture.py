# Python closures look the variable up when they are CALLED (late binding),
# which behaves like C++ capture by reference.
fs = [lambda: i for i in range(3)]
print([f() for f in fs])

# The default-argument trick takes the value NOW, like C++ capture by value.
gs = [lambda i=i: i for i in range(3)]
print([g() for g in gs])
