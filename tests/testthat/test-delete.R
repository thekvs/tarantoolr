test_that("delete method works", {
    system("tarantoolctl eval example cleanup.lua")
    system("tarantoolctl eval example init.lua")
    system("tarantoolctl eval example populate_db.lua")

    tnt <- new(Tarantool)
    expect_that(tnt$ping(), is_true())

    res <- tnt$delete("test", 2L, NULL)
    expect_that(length(res), equals(1))
    expect_that(res[[1]], equals(list(2, "bbb")))

    res <- tnt$select("test", NULL, NULL)
    expect_that(length(res), equals(5))
    # expect_that(res, equals(list(list(1, "aaa"), list(3, "ccc"), list(4, "ddd"))))

    system("tarantoolctl eval example cleanup.lua")
})
