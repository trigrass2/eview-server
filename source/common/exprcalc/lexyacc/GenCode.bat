del lexer.cpp parser.h parser.cpp y_tab.* y.output

rem ��syntex.h����y_tab.h/y_tab.c/y.output�����ļ�
bison -y -d -v syntex.y

rename y_tab.h parser.h
rename y_tab.c parser.cpp

rem ��lex.l����lexer.cpp
flex  -i -t lex.l > lexer.cpp

rem ��lexer.cpp��parser.h��parser.cpp�������ϼ�Ŀ¼���и���
rem ����ע�͵�lex.cpp�е�#include <unistd.h> һ��