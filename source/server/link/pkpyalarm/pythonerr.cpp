#include "pythonerr.h"
#include "python/Python.h"
#include "pklog/pklog.h"

extern CPKLog g_logger;


string log_python_exception()
{
	std::string strErrorMsg = "";
	if (!Py_IsInitialized())
	{
		strErrorMsg = "Python 运行环境没有初始化！";
		return strErrorMsg;
	}

	if (PyErr_Occurred() != NULL)
	{
		PyObject *type_obj, *value_obj, *traceback_obj;
		PyErr_Fetch(&type_obj, &value_obj, &traceback_obj);
		if (value_obj == NULL)
			return strErrorMsg;

		PyErr_NormalizeException(&type_obj, &value_obj, 0);
		if (PyString_Check(PyObject_Str(value_obj)))
		{
			strErrorMsg = PyString_AsString(PyObject_Str(value_obj));
		}

		if (traceback_obj != NULL)
		{
			strErrorMsg += "Traceback:";

			PyObject * pModuleName = PyString_FromString("traceback");
			PyObject * pTraceModule = PyImport_Import(pModuleName);
			Py_XDECREF(pModuleName);
			if (pTraceModule != NULL)
			{
				PyObject * pModuleDict = PyModule_GetDict(pTraceModule);
				if (pModuleDict != NULL)
				{
					PyObject * pFunc = PyDict_GetItemString(pModuleDict, "format_exception");
					if (pFunc != NULL)
					{
						PyObject * errList = PyObject_CallFunctionObjArgs(pFunc, type_obj, value_obj, traceback_obj, NULL);
						if (errList != NULL)
						{
							int listSize = PyList_Size(errList);
							for (int i = 0; i < listSize; ++i)
							{
								strErrorMsg += PyString_AsString(PyList_GetItem(errList, i));
							}
						}
					}
				}
				Py_XDECREF(pTraceModule);
			}
		}
		Py_XDECREF(type_obj);
		Py_XDECREF(value_obj);
		Py_XDECREF(traceback_obj);
	}
	return strErrorMsg;
}

void LogPythonError()
{
	string strErrorMsg = log_python_exception();
	if (!strErrorMsg.empty())
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "python execute failed,error:%s", strErrorMsg.c_str());
}