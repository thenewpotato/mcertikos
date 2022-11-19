#!/bin/bash

fswatch -or . |
while
  rsync -Pa -e "ssh -i $HOME/.ssh/id_zoo" --stats ./ jw2723@node.zoo.cs.yale.edu:~/mcertikos
  read f
do :; done