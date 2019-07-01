Param MONAA --- A Tool for Parametric Timed Patten Matching with Automata-Based Acceleration
============================================================================================

[![wercker status](https://app.wercker.com/status/062aedaff5135bc38b9fd7730d43aed6/s/master "wercker status")](https://app.wercker.com/project/byKey/062aedaff5135bc38b9fd7730d43aed6)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](./LICENSE)

This is the source code repository for Param MONAA --- A Tool for Parametric Timed Patten Matching with Automata-Based Acceleration.

The demo on Google Colab is [HERE](https://colab.research.google.com/drive/1JQtKtMWBqCn1xoD9iE_k7rlGHSrCCGwn)!!

Usage
-----

### Synopsis

    pmonaa [OPTIONS] -f FILE

### Options

**-h**, **--help** Print a help message. <br />
**-q**, **--quiet** Quiet mode. Causes any results to be suppressed. <br />
**-s** *mode*, **--skip** *mode* Specify which skipping is used. It should be one of `parametric` (default), `non-parametric`, and `none`. <br />
**-a**, **--ascii** Ascii mode. (default) <br />
**-b**, **--binary** Binary mode. <br />
**-V**, **--version** Print the version <br />
**-E**, **--event** Event mode (default) <br />
**-S**, **--signal** Signal mode <br />
**-i** *file*, **--input** *file* Read a timed word from *file*. <br />
**-f** *file*, **--automaton** *file* Read a timed automaton from *file*. <br />
**-e** *pattern*, **--expression** *pattern* Specify a *pattern* by a timed regular expression. <br />

Example
-------
    
    ./build/pmonaa -s parametric -f ./example/pta.dot < ./example/timed_word.txt


Installation
------------

Param MONAA is tested on Arch Linux and Mac OSX 10.14.1

### Requirements

* C++ compiler supporting C++14 and the corresponding libraries.
* Boost (>= 1.59)
* Eigen
* CMake
* Bison
* Flex
* PPL

### Instructions

```sh
mkdir build 
mkdir build/tre build/constraint
cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install
```

Syntax of Parametric Timed Automata
-----------------------------------

You can use [DOT language](http://www.graphviz.org/content/dot-language) to represent a parametric timed automaton. For the timing constraints and other information, you can use the following custom attributes.

<table>
<thead>
<tr class="header">
<th></th>
<th>attribute</th>
<th>value</th>
<th>description</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td>vertex</td>
<td>init</td><td>0 or 1</td><td><tt>init=1</tt> if the state is initial</td></tr>
<tr class="even">
<td>vertex</td><td>match</td><td>0 or 1</td><td><tt>match=1</tt> if the state is accepting</td>
</tr>
<tr class="odd">
<td>edge</td><td>label</td><td>[a-z], [A-Z]</td><td>the value represents the event on the transition</td>
</tr>
<tr class="even">
<td>edge</td><td>reset</td><td>a list of integers</td><td>the set of variables reset after the transition</td>
</tr>
<tr class="odd">
<td>edge</td><td>guard</td><td>a list of inequality constraints</td><td>the guard of the transition</td>
</tr>
</tbody>
</table>

References
-------------

* Offline Timed Pattern Matching under Uncertainty. Étienne André, Ichiro Hasuo, and Masaki Waga. ICECCS 2018: 10-20 [[arXiv version]](https://arxiv.org/abs/1812.08940)
* Online Parametric Timed Pattern Matching with Automata-Based Skipping. Masaki Waga and Étienne André, NFM 2019: 371-389 [[arXiv version]](https://arxiv.org/abs/1903.07328)
