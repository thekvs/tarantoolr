## Tarantoolr
R driver for [Tarantool](https://github.com/tarantool/tarantool) 1.6+ in-memory database and application server.

## Install
As for now `tarantoolr` package can only be installed directly from GitHub using `devtools` package.
To install `tarantoolr` you should execute in R's shell following commands:
```
install.packages('devtools')
library(devtools)
devtools::install_github('thekvs/tarantoolr')
```

## Usage
Obligatory "Hello, World!" example:
```
> library(tarantoolr)
> tnt <- new(Tarantool)
> res1 <- tnt$insert("example", list(2016L, 3.14, TRUE, FALSE, NULL, list("x", "y", "z")))
> res2 <- tnt$select("example", 2016L, NULL)
> all.equal(res1, res2)
[1] TRUE
> str(res1)
List of 1
 $ :List of 6
  ..$ : num 2017
  ..$ : num 3.14
  ..$ : logi TRUE
  ..$ : logi FALSE
  ..$ : NULL
  ..$ :List of 3
  .. ..$ : chr "x"
  .. ..$ : chr "y"
  .. ..$ : chr "z"
```

Some other usage examples can be found in [tests](tests/testthat) directory.
