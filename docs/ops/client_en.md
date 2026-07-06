# Clients (Fully consistent with Redis — all clients supported by Redis are supported by Pika)

★ denotes the recommended client for that language.

## ActionScript

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| as3redis | [Repository](https://github.com/claus/as3redis) | [cwahlers](http://twitter.com/cwahlers) | |

## C

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| hiredis ★ | [Repository](https://github.com/antirez/hiredis) | [antirez](http://twitter.com/antirez) [pnoordhuis](http://twitter.com/pnoordhuis) | Official C client. Supports all set commands, pipelining, and event-driven programming. |
| credis | [Repository](http://code.google.com/p/credis/source/browse) | | |
| libredis | [Repository](https://github.com/toymachine/libredis) | | Supports parallel execution of commands on multiple servers via polling and ketama hashing. |

## C#

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| ServiceStack.Redis ★ | [Homepage](https://github.com/ServiceStack/ServiceStack.Redis) | [demisbellot](http://twitter.com/demisbellot) | An enhanced fork of Miguel De Icaza's C# client. |
| Booksleeve ★ | [Homepage](http://code.google.com/p/booksleeve/) | [marcgravell](http://twitter.com/marcgravell) | High-performance client using heap exchange. |
| Sider | [Homepage](http://nuget.org/List/Packages/Sider) | [chakrit](http://twitter.com/chakrit) | Minimal client for .NET 4.0. |
| TeamDev Redis Client | [Repository](http://redis.codeplex.com/) | [TeamDevPerugia](http://twitter.com/TeamDevPerugia) | Based on redis-sharp, provides basic communication functionality with some differences. |
| redis-sharp | [Repository](https://github.com/migueldeicaza/redis-sharp) | [migueldeicaza](http://twitter.com/migueldeicaza) | |

## C++

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| C++ Client | [Repository](https://github.com/mrpi/redis-cplusplus-client) | | |
| redis3m | [Repository](https://github.com/luca3m/redis3m) | [luca3m](http://twitter.com/luca3m) | Modern C++ wrapper for hiredis. |
| redisclient | [Repository](https://github.com/nekipelov/redisclient) | [nekipelov](https://github.com/nekipelov) | Asynchronous client based on Boost.Asio. |
| SimpleRedisClient | [Repository](https://github.com/Levhav/SimpleRedisClient) | | |

## Clojure

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| carmine ★ | [Repository](https://github.com/ptaoussanis/carmine) | [ptaoussanis](http://twitter.com/ptaoussanis) | Modern Clojure client with clustering support. |
| clj-redis | [Repository](https://github.com/mmcgrana/clj-redis) | [mmcgrana](http://twitter.com/mmcgrana) | Based on Jedis. |
| redis-clojure | [Repository](https://github.com/tavisrudd/redis-clojure) | [tavisrudd](http://twitter.com/tavisrudd) | Pure Clojure client. |

## ColdFusion (CFML)

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| ColdFusion Redis SDK | [Homepage](https://github.com/ghidinelli/cfredis) | [ghidinelli](http://twitter.com/ghidinelli) | |

## Common Lisp

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| cl-redis | [Repository](https://github.com/vseloved/cl-redis) | [vseloved](http://twitter.com/vseloved) | |
| redis | [Repository](https://github.com/marekjeszka/redis-cl-client) | | |

## Dart

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| dart_redis | [Repository](https://github.com/ra1u/dart-redis) | | Dart client. |

## Emacs Lisp

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| emacs-redis | [Repository](https://github.com/emacsmirror/redis) | [ghostramses](http://twitter.com/ghostramses) | |

## Erlang

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| Eredis ★ | [Repository](https://github.com/wooga/eredis) | [Wooga](http://twitter.com/wooga) | |
| erldis | [Repository](https://github.com/alessio/erldis) | [alessio](http://twitter.com/alessio) | |
| redis-erl | [Repository](https://github.com/Etsy/redis-erl) | [Etsy](http://twitter.com/Etsy) | |

## Fancy

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| Fancy Redis Client | [Repository](https://github.com/bakkdoor/fancy-redis) | | |

## Go

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| Redigo ★ | [Repository](https://github.com/garyburd/redigo) | [garyburd](http://twitter.com/garyburd) | |
| Go-redis | [Repository](https://github.com/fzzbt/radix) | | |
| redis | [Repository](https://github.com/hoisie/redis) | [hoisie](http://twitter.com/hoisie) | |

## Haskell

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| hedis ★ | [Repository](https://github.com/informatikr/hedis) | [informatikr](http://twitter.com/informatikr) | |
| redis-haskell | [Repository](https://github.com/tmhedberg/redis-haskell) | | |

## haXe

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| hxRedis | [Repository](https://github.com/ProG4mr/hxredis) | | |

## Java

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| Jedis ★ | [Repository](https://github.com/xetorthio/jedis) | [xetorthio](http://twitter.com/xetorthio) | |
| lettuce ★ | [Repository](https://github.com/mp911de/lettuce) | [mp911de](http://twitter.com/mp911de) | Advanced thread-safe client for regular Redis, Redis Sentinel, and Redis Cluster. |
| Redisson ★ | [Repository](https://github.com/mrniko/redisson) | [mrniko](http://twitter.com/mrniko) | Scalable and thread-safe client, based on Netty framework. Over 20 Redis data structures and services. |
| JDBC-Redis | [Repository](http://code.google.com/p/jdbc-redis/) | | |
| JRedis | [Repository](https://github.com/alphazero/jredis) | [alphazero](http://twitter.com/alphazero) | |
| RJC | [Repository](https://github.com/e-mzungu/rjc) | | |
| spring-data-redis | [Repository](http://projects.spring.io/spring-data-redis/) | | |

## JavaScript (Node.js)

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| node_redis ★ | [Repository](https://github.com/mranney/node_redis) | [mranney](http://twitter.com/mranney) | |
| ioredis ★ | [Repository](https://github.com/luin/ioredis) | [luin](http://twitter.com/luin) | Full-featured client with clustering and sentinel support. |
| redis-js | [Repository](https://github.com/fictorial/redis-node-client) | [fictorial](http://twitter.com/fictorial) | |

## Julia

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| Redis.jl | [Repository](https://github.com/jkaye2012/Redis.jl) | | Pure Julia implementation. |

## Lua

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| redis-lua ★ | [Repository](https://github.com/nrk/redis-lua) | [nrk](http://twitter.com/nrk) | |

## Matlab

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| redis-matlab | [Repository](https://github.com/markuman/go-redis) | [markuman](http://twitter.com/markuman) | |

## .NET / C#

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| StackExchange.Redis ★ | [Repository](https://github.com/StackExchange/StackExchange.Redis) | [StackExchange](http://twitter.com/StackExchange) | High-performance .NET client. |

## Objective-C

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| ObjCHiredis ★ | [Repository](https://github.com/lp/ObjCHiredis) | [lp](https://github.com/lp) | |
| MPJRedis | [Repository](https://github.com/mpjHQ/MPJRedis) | | |

## OCaml

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| redis-ocaml | [Repository](https://github.com/0xffea/redis-ocaml) | | |

## Perl

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| Redis ★ | [Repository](https://metacpan.org/pod/Redis) | [PetruEng](http://twitter.com/PetruEng) | |
| Redis::Fast | [Repository](https://metacpan.org/pod/Redis::Fast) | | Perl extension that wraps hiredis. |

## PHP

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| phpredis ★ | [Repository](https://github.com/phpredis/phpredis) | [nicolasff](http://twitter.com/nicolasff) | PHP Extension, C code. Fastest PHP client. |
| Predis ★ | [Repository](https://github.com/nrk/predis) | [nrk](http://twitter.com/nrk) | Pure PHP client. |

## Python

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| redis-py ★ | [Repository](https://github.com/andymccurdy/redis-py) | [andymccurdy](http://twitter.com/andymccurdy) | |
| aioredis | [Repository](https://github.com/aio-libs/aioredis) | | Async Redis client. |
| txRedis | [Repository](https://github.com/deldotdr/txRedis) | [deldotdr](http://twitter.com/deldotdr) | Twisted-based client. |

## R

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| rredis | [Repository](http://cran.r-project.org/web/packages/rredis/index.html) | | |

## Racket

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| redis-racket | [Repository](https://github.com/stchang/redis) | | |

## Rebol

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| Rebol Redis | [Repository](https://github.com/rebolek/prot-redis) | [rebolek](http://twitter.com/rebolek) | |

## Ruby

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| redis-rb ★ | [Repository](https://github.com/redis/redis-rb) | | Official Ruby client. |
| redis-namespace | [Repository](https://github.com/resque/redis-namespace) | | |

## Rust

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| redis-rs ★ | [Repository](https://github.com/mitsuhiko/redis-rs) | [mitsuhiko](http://twitter.com/mitsuhiko) | |

## Scala

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| scala-redis ★ | [Repository](https://github.com/debasishg/scala-redis) | [debasishg](http://twitter.com/debasishg) | |
| Sedis | [Repository](https://github.com/pk11/sedis) | [pk11](http://twitter.com/pk11) | Wrapper for Jedis. |
| rediscala | [Repository](https://github.com/etaty/rediscala) | [etaty](http://twitter.com/etaty) | Non-blocking client for Redis based on Akka. |

## Scheme

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| redis-scheme | [Repository](https://github.com/ves/redis-scheme) | | |

## Swift

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| Zewo/Redis | [Repository](https://github.com/Zewo/Redis) | [Zewo](https://github.com/Zewo) | |

## Tcl

| Client | Repository | Author | Notes |
|--------|-----------|--------|-------|
| redis tcl | [Repository](http://github.com/antialize/redis-tcl) | | |
| Ric | [Repository](https://github.com/mjsottile/ric) | | |
