Glasgow Automaton Programming Language (GAPL)
=============================================

The keystone of this unified system is a topic-based, publish/subscribe cache. Topics are organised in memory as either append-only stream tables or static relational tables. Ad hoc select queries, enhanced with time windows, can be presented to the cache at any time. In imperative programming language (the Glasgow Automaton Programming Language, GAPL) is used to program automata to detect complex event patterns over the cached streams and relations. When registered against the cache, each automaton subscribes to chosen topics and receives each event inserted into those topics through the publish/subscribe infrastructure for further processing. Automata can also access (modify) the relational tables, publish new tuples into stream tables, and send events to external processes.

General form
------------
The general form of an automaton is:

1. subscription(s)
2. association(s)
3. declaration(s)
4. initialisation clause
5. behaviour clause

Basic Data Types
----------------

Type | Description
-----|------------
**int** | 64-bit integer
**real** | Double-precision floating point
**tstamp** | 64-bit unsigned integer (ns since the epoch)
**bool** | True or False
**string** | Variable-length UTF8 array

Complex Data Types
------------------

Type | Description
-----|------------
**sequence** | Ordered set of heterogeneous data type instances
**map** | Maps from an identifier to an instance of the bound type
**window** | Collection of bound type instances that is constrained either to a fixed number of items or a fixed time interval
**identifier** | Key used in maps
**iterator** | Used to iterate over all instances in a map (keys) or window (data values)
**event<topicName>** | Used to store a single event instance locally

Functions
---------
### Cache functions
__string__ *currentTopic*()
 - returns the current Topic which triggered the activation of the behaviour clause, useful when subscribed to more than one topic

__void__ *send*(arg, ...) 
 - sends the provided arguments back to the process which registered the automaton

__void__ *publish*(topic, arg, ...)
 - publishes the args to the specified topic

### General functions
- real float(int)
- string String(arg[, ...])
- void topOfHeap()

### Map (association) functions
__identifier__ *Identifier*(arg[, ...])
 - returns a newly created key based on the arg for querying maps

__bool__ *hasEntry*(map, identifier)
 - returns True if the key (identifier) exists at least once in the map

__map__.type__ *lookup*(map, identifier)
 - returns the value associated with the key (identifier)

__int__ *mapSize*(map)
 - returns the size of the map

__void__ *insert*(map, ident, map.type)
 - inserts a new key, value pair into the map

__void__ *remove*(map, ident)
 - removes the key, value pair in the map

__void__ *frequent*(map, ident, int)

### Sequence functions
__sequence__ *Sequence*([arg[, ...]])
 - returns a newly created sequence containing the args provided

__void__ *append*(sequence, basictype[, ...])
 - appends an item to an existing sequence

__basictype__ *seqElement*(sequence, int)
 - returns the element stored at the index in the sequence

__int *seqSize*(sequence)
 - returns the size of the sequence

### Window functions
- real average(window)
- real stdDev(window)
- int winSize(window)
- sequence lsqrf(window)
- real winMax(window)
- void append(window, window.dtype[, tstamp])


### Iterator functions
- iterator Iterator(map|window|sequence)
- identifier|data next(iterator)
- bool hasNext(iterator)

### Time functions
- tstamp tstampNow()
- tstamp tstampDelta(tstamp, int, bool)
- int tstampDiff(tstamp, tstamp)
- tstamp Timestamp(string)
- int dayInWeek(tstamp) [Sun/0,Sat/6]
- int hourInDay(tstamp) [0..23]
- int dayInMonth(tstamp) [1..31]
- int secondInMinute(tstamp) [0..60]
- int minuteInHour(tstamp) [0..59]
- int monthInYear(tstamp) [1..12]
- int yearIn(tstamp) [1900 .. ]


### Network functions
- int IP4Addr(string)
- int IP4Mask(int)
- bool matchNetwork(string, int, int)

### Real functions
- real power(real, real)
- int floor(real)

