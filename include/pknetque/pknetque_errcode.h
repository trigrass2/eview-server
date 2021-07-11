#ifndef ERROR_CODE_PKQUE_H_
#define ERROR_CODE_PKQUE_H_

#ifndef EC_PKQUE_BASENO
#define EC_PKQUE_BASENO					13000 // iCV5�������������ʼ
#define EC_PKQUE_INITFAILED				EC_PKQUE_BASENO + 10			// PKSOCK��ʼ��ʧ��
#define EC_PKQUE_LISTENFAILED				EC_PKQUE_BASENO + 11			// PKSOCK�����˿�ʧ��
#define EC_PKQUE_SENDFAILED				EC_PKQUE_BASENO + 12			// ��������ʧ��
#define EC_PKQUE_RECVFAILED				EC_PKQUE_BASENO + 13			// ��������ʧ��
#define EC_PKQUE_FINALIZEFAILED			EC_PKQUE_BASENO + 14			// ��ֹCVNDK����ʧ��
#define EC_PKQUE_QUEUEERROR				EC_PKQUE_BASENO + 15			// PKSOCK�е�Queue��ش���
#define EC_PKQUE_CONNECTFAILED				EC_PKQUE_BASENO + 16			// ����ʧ��
#define EC_PKSOCKK_GET_PORT_FAILED			EC_PKQUE_BASENO + 17			// ��ȡָ��Ӧ�õĶ˿�ʧ��
#define EC_PKQUE_RECV_TIMEOUT				EC_PKQUE_BASENO + 18			// �������ݳ�ʱ
#define EC_PKQUE_NOT_INIT					EC_PKQUE_BASENO + 19			// PKSOCK��δ��ʼ��
#define EC_PKQUE_INVALID_PARAMETER			EC_PKQUE_BASENO + 20			// ��Ч�Ĳ�������
#define EC_PKQUE_MALLOC_FAILED				EC_PKQUE_BASENO + 21			// ����ռ�ʧ��
#define EC_PKQUE_PUTQ_FAILED				EC_PKQUE_BASENO + 22			// ���ݷ��͵�ȫ��дQueueʧ��
#define EC_PKQUE_LOCAL_QUEUE_NOT_FOUND		EC_PKQUE_BASENO + 23			// ����Queueδ���ҵ�
#define EC_PKQUE_CONNECT_NOT_EXIST			EC_PKQUE_BASENO + 24			// ��Զ��Queue�����Ӳ�����
#define EC_PKQUE_ACQUIRE_MUTEX_FAILED		EC_PKQUE_BASENO + 25			// ��ȡȫ����ʧ��
#endif // #ifndef EC_PKQUE_COMM_BASENO

#endif//ERROR_CODE_CVNDK_H_
