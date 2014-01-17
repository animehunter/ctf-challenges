CTF Challenge Solutions
=======================

Week 1 (Silly Jimmy)
--------------------
In this challenge, the goal was to analyze a python script and reverse engineer a 32-Bit Linux executable.

One of the solution:
```
perl -e 'print "\xF6\xFF\xFF\xFF\x79\x90\x6d\x90\x6d\x90\x69\x90\x6a\x00\x79\x90\x6d\x90\x6d\x90\x69\x90\x6a\x00\x79\x90\x6d\x90\x6d\x90\x69\x90\x00\x00\x00\x00\x01\x02\x03\x04\x01\x02\x03\x04\x3b\xc5\xc5\xc5" ' | nc 127.0.0.1 7331
```

Flag {dammit_I_thought_xor_encrpytion_was_unbreakable!} 

Week 2
------
Coming Soon...
