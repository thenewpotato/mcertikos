#!/bin/bash

#fswatch -or . |
#while
#  rsync -Pa -e "ssh -i $HOME/.ssh/id_zoo" --stats ./ jw2723@node.zoo.cs.yale.edu:~/mcertikos
#  read f
#do :; done

if [ "$1" == "to" ]; then rsync -Pzar -e "ssh -i $HOME/.ssh/id_zoo" ./ jw2723@node.zoo.cs.yale.edu:~/mcertikos; fi
if [ "$1" == "from" ]; then rsync -Pzar -e "ssh -i $HOME/.ssh/id_zoo" jw2723@node.zoo.cs.yale.edu:~/mcertikos ../; fi
