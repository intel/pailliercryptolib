for item in $(locate QAT/build); do [ -d $item ] && [ $(echo $item | grep $HOME) ] && echo ${item%/*}; done
