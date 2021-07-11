#ifndef ERROR_CODE_PKSOCK_H_
#define ERROR_CODE_PKSOCK_H_

#ifndef EC_PKSOCK_BASENO
#define EC_PKSOCK_BASENO					13000 // iCV5�������������ʼ
#define EC_PKSOCK_INITFAILED				EC_PKSOCK_BASENO + 10			// PKSOCK��ʼ��ʧ��
#define EC_PKSOCK_LISTENFAILED				EC_PKSOCK_BASENO + 11			// PKSOCK�����˿�ʧ��
#define EC_PKSOCK_SENDFAILED				EC_PKSOCK_BASENO + 12			// ��������ʧ��
#define EC_PKSOCK_RECVFAILED				EC_PKSOCK_BASENO + 13			// ��������ʧ��
#define EC_PKSOCK_FINALIZEFAILED			EC_PKSOCK_BASENO + 14			// ��ֹCVNDK����ʧ��
#define EC_PKSOCK_QUEUEERROR				EC_PKSOCK_BASENO + 15			// PKSOCK�е�Queue��ش���
#define EC_PKSOCK_CONNECTFAILED				EC_PKSOCK_BASENO + 16			// ����ʧ��
#define EC_PKSOCKK_GET_PORT_FAILED			EC_PKSOCK_BASENO + 17			// ��ȡָ��Ӧ�õĶ˿�ʧ��
#define EC_PKSOCK_RECV_TIMEOUT				EC_PKSOCK_BASENO + 18			// �������ݳ�ʱ
#define EC_PKSOCK_NOT_INIT					EC_PKSOCK_BASENO + 19			// PKSOCK��δ��ʼ��
#define EC_PKSOCK_INVALID_PARAMETER			EC_PKSOCK_BASENO + 20			// ��Ч�Ĳ�������
#define EC_PKSOCK_MALLOC_FAILED				EC_PKSOCK_BASENO + 21			// ����ռ�ʧ��
#define EC_PKSOCK_PUTQ_FAILED				EC_PKSOCK_BASENO + 22			// ���ݷ��͵�ȫ��дQueueʧ��
#define EC_PKSOCK_LOCAL_QUEUE_NOT_FOUND		EC_PKSOCK_BASENO + 23			// ����Queueδ���ҵ�
#define EC_PKSOCK_CONNECT_NOT_EXIST			EC_PKSOCK_BASENO + 24			// ��Զ��Queue�����Ӳ�����
#define EC_PKSOCK_ACQUIRE_MUTEX_FAILED		EC_PKSOCK_BASENO + 25			// ��ȡȫ����ʧ��
#endif // #ifndef EC_PKSOCK_COMM_BASENO

#endif//ERROR_CODE_CVNDK_H_
