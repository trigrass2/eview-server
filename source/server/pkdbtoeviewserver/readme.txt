配置文件：ini格式
database:数据库连接参数（dbtype、connection、username、password）；period为轮询数据库的周期（单位为秒，仅支持一个周期）
eview：eview服务的ip
tags:等号后执行数据库SQL语句第一行第一列的值（转为字符串类型），保存到等号前的eview tag变量中
样例：

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