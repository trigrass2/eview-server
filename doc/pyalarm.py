def OnAlarm(alarm):  
    sql=''
    sql+=alarm["ProduceTime"]  
    sql+=alarm["Priority"] 
    sql+=alarm["Subsys"]  
    sql+=alarm["AlarmName"] 
    sql+=alarm["AlarmCName"]
    sql+=alarm["AlarmValue"]
    sql+=alarm["DeviceName"]
    return sql
