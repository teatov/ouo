## Variables

- [ ] variable declarations

```ouo
var ass: int = 69
var blast = 420
```

- [ ] constant declarations

```ouo
const usa: float = 1337.0
```

## Types

- [ ] type declarations

```ouo
type Id: int
var n: Id = 5
```

### Scalar types

- [x] integer

```ouo
var i: int = 10
```

- [ ] underscores in numbers

```ouo
var i: int = 1_000_000
```

- [x] float

```ouo
var f: float = 3.14
```

- [ ] scientific notation

```ouo
var f: float = 314e-2
```

- [ ] string

```ouo
var s: str = 'ass'
var s: str = "ass"
```

- [ ] template string

```ouo
var t: str = "ass \(i + 5)"
var t2: str = "ass \(if (i > 2) {) blast \(})"
```

- [ ] raw string

```ouo
var s: str = #"ass\n"#
```

- [ ] multiline string

```ouo
var s: str = `ass
  	         `blast
  	         `usa
```

- [ ] boolean

```ouo
var b: bool = true
var b: bool = false
```

- [ ] byte

```ouo
var x: byte = 64
```

- [ ] binary and hex literals

```ouo
var x: byte = 0b01010101
var x: byte = 0xBADA55
```

### Sum types

- [ ] union types

```ouo
type Number = un[int, float]
var ni: Number = 5
var nf: Number = 5.0

fn ass(blast: un[Number, str, bool]) {}
ass(ni)
ass('usa')
```

- [ ] optional types

```ouo
type MaybeInt = un[int, none]
type MaybeInt = opt[int]
```

### Composite types

- [ ] array

```ouo
var a = arr[int][1, 2, 3]
var e = a[0]

var aa: arr[arr[int]] = [
	[1, 2, 3],
	[4, 5, 6],
	[7, 8, 9],
]
```

- [ ] record

```ouo
var s = rec[ass: int][.ass: 5]

type Point = rec[
	x: int,
	y: int,
]

var p = Point[
	.x: 5,
	.y: 10,
]
```

### Literal types

- [ ] literal types

```ouo
type Zero: 0
type HttpMethod: un["GET", "POST"]
```

## Control flow

- [ ] if expressions

```ouo
if (a) {
	print('a')
} else if (b) {
	print('b')
} else {
	print('else')
}
```

- [ ] switch expressions

```ouo
switch (n) {
	1 => print('one')
	2 => print('two')
	else => {
		print('???')
	}
}
```

- [ ] switching on union types

```ouo
var a: un[int, str] = 5

switch (a) {
	int => print('int', a)
	str => print('str', a)
}

switch (a) int => print('int')
```

## Functions

- [ ] functions

```ouo
fn ass(blast: int): int {
	var usa := blast + 5
	return usa
}

fn ass(blast: int) {
	print(blast + 5)
}

fn ass(blast: int) blast + 5
```

## Methods

- [ ] methods

```ouo
type Number: int {
	fn sum(self, other: int) self + other
}

var n: Number = 5
n.sum(10)
```

- [ ] constructors

```ouo
type Vec2: rec[
	x: float,
	y: float,
] {
	fn (x: float, y: float) Vec2[.x: x, .y: y]
}

var v = Vec2(3.0, 4.0)
```

- [ ] static methods

```ouo
type Vec2: rec[
	x: float,
	y: float,
] {
	fn double(v: Vec2): Vec2 {
		return Vec2[.x: v.x * 2.0, .y: v.y * 2.0]
	}
}

var v = Vec2[.x: 1.0, .y: 2.0]
Vec2.double(v)
```

## Traits

- [ ] traits

```ouo
type Shape: trait[
	fn area(self): float,
]

type Circle: rec[r: float] impl Shape {
	fn area(self) 3.14 * self.r * self.r
}

type Rect: rec[
	w: float,
	h: float,
] impl Shape {
	fn area(self) self.w * self.h
}

fn print_area(s: Shape) {
	print(s.area())
}

print_area(Circle[.r: 1.0])
print_area(Rect[.w: 2.0, .h: 3.0])
```

## Operators

- [ ] checking optional types

```ouo
if (?a) {
	print('a is not none')
} else {
	print('a is none')
}
```

- [ ] approximate float equality

```ouo
a ~= 1.0
```

## Modules

- [ ] module import

```ouo
use ./relative_dir/my_module
use /cwd_dir/my_module
use @std/std_module
use @std/std_module as my_module

my_module::PI
my_module::do_somethind()
```

- [ ] module export

```ouo
pub const PI = 3.14
pub fn do_somethind() {
  print("something!")
}
```

- [ ] template module

````ouo
templ article: rec[title: str, date: Date, body: str]

var date_fmt: str = article.date.formatted()

```html
<h1>\(article.title)</h1>
<p>\(date_fmt)</p>
<p>\(article.body)</p>
````

```ouo
use article_templ

var article = [
  .title: "my article",
  .date: Date(2000, 1, 1),
  .body: "hiiiiii :)",
]
var a: str = article_templ(article)
```
