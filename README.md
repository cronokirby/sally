# Sally

Sally is an attempt at making a toy shell, written in C. My main goal here
was to learn how to write a shell, and the low level C UNIX APIs.

# Building

Standard command:

```
make release
```

A debug build is also available with:

```
make debug
```

These place the executable `sally` in `target/release` and `target/debug`,
respectively.

# Running

The command `sally` should work mostly like how `sh` does,
although taking no initial arguments when starting.

# Features

## Builtins

**pwd**:

```
>> pwd
```

Prints the current directory.

**cd**:

```
>> cd ..
```

Used to change directories.

## Launching Programs

```
>> echo foo bar baz
```

Arbitrary programs can be launched inside of the shell, with arguments.


## Redirection

Stdout can be redirected to a file:

```
>> echo foo > foo.txt
```

## Pipes

Pipes are also supported, including with builtins:

```
>> pwd | wc -c
```
