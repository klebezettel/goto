goto - Bookmarks for the shell
==============================

A cdargs clone, implemented with C++11 and ncurses.

Build instructions
------------------
		$ qmake  # You may need to install qt/qmake.
		$ make

Though qmake is used, there is no run time dependency to any Qt library.

Setup instructions
------------------
1. Make the binary invokable by setting a proper PATH.
2. Use the following shell function as a wrapper. You may want to bind the
   function to a short cut as in the example below.

		# Call this from your shell to access your bookmarks.
		function to()
		{
		  goto

		  # If the user aborts by Ctrl-C there will be no result file, so check first.
		  result_file=$HOME/.goto.result
		  if [ -f "$result_file" ]; then
			result_path=`cat $result_file`
			rm $result_file
		  else
			result_path="."
		  fi
		  cd $result_path;
		}

		# Convenience, bind to() to Alt+`
		# This is zsh specific. Adapt to your shell.
		# ^q: Evaluate the next stuff isolated from so far entered text.
		bindkey -s '`' '^qto\n' 

Run instructions
----------------
Just call the shell function directly:

 $ to

or hit Ctrl-F.
