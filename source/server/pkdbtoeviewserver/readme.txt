�����ļ���ini��ʽ
database:���ݿ����Ӳ�����dbtype��connection��username��password����periodΪ��ѯ���ݿ�����ڣ���λΪ�룬��֧��һ�����ڣ�
eview��eview�����ip
tags:�Ⱥź�ִ�����ݿ�SQL����һ�е�һ�е�ֵ��תΪ�ַ������ͣ������浽�Ⱥ�ǰ��eview tag������
������

[database]
#dbtype=sqlite
#connection=eview.db
#dbtype=sqlserver
#connection=192.168.10.1:1433@master
dbtype=mysql
connection=127.0.0.1:3306@eview_pudong
username=root
password=root
period=10

[eview]
serverip=127.0.0.1

[tags]
tag1=SELECT (SUM(DATA_LENGTH)+SUM(INDEX_LENGTH))/1024/1024/1024 as data_sum FROM information_schema.tables
tag2=select * from t_device_list where 1=1