del lexer.cpp parser.h parser.cpp y_tab.* y.output

rem 由syntex.h生成y_tab.h/y_tab.c/y.output三个文件
bison -y -d -v syntex.y

rename y_tab.h parser.h
rename y_tab.c parser.cpp

rem 由lex.l生成lexer.cpp
flex  -i -t lex.l > lexer.cpp

rem 将lexer.cpp和parser.h、parser.cpp拷贝到上级目录进行覆盖
rem 并请注释掉lex.cpp中的#include <unistd.h> 一行