#include "console.h"
#include <ctime>

COORD getXY()							//ͨ��WindowsAPI������ȡ����λ��
{
	CONSOLE_SCREEN_BUFFER_INFO pBuffer;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &pBuffer);
	//���ñ�׼��������ù��������Ϣ

	return COORD{ pBuffer.dwCursorPosition.X, pBuffer.dwCursorPosition.Y };
	//��װΪ��ʾ�����COORD�ṹ
}

COORD getScrnInfo()										//��ȡ����̨���ڻ�������С
{
	HANDLE hStd = GetStdHandle(STD_OUTPUT_HANDLE);		//��ñ�׼����豸���
	CONSOLE_SCREEN_BUFFER_INFO scBufInf;				//����һ�����ڻ�������Ϣ�ṹ��

	GetConsoleScreenBufferInfo(hStd, &scBufInf);		//��ȡ���ڻ�������Ϣ

	return scBufInf.dwSize;								//���ش��ڻ�������С
}

void moveXY(COORD pstn)
{
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pstn);
	//ͨ����׼���������ƹ���λ��
}

void clearDisplay(COORD firstPst, COORD lastPst)		//���������Ļ���ݣ���firstPst���굽lastPst����֮�������
{
	int yDValue(lastPst.Y - firstPst.Y);					//��¼��ĩλ���������ֵ�����Ƶ�������

	COORD size(getScrnInfo());								//��¼Ŀǰ����̨��������С

	moveXY(firstPst);							//�ƶ���굽��λ��
	for (int y(0); y <= yDValue; y++)			//һ��ѭ�������������
	{
		for (int x(firstPst.X); x <= size.X; x++)			//����ѭ�������ظ����
		{
			std::cout << ' ';						//���һ���ո�������ԭ���ݣ��ﵽ���Ч��
			int px;									//��¼��굱ǰλ�õĺ�����
			if (x != size.X)
				px = x + 1;
			else
				px = 0;
			if (y == yDValue && px == lastPst.X)		//����ĩλ�����Աȣ��ﵽĩλ�ü��˳�ѭ��
				break;
		}
	}
	moveXY(firstPst);
}

void loading(float process,const char* desc1,const char* desc2)									//�ȴ����棬ģ�⶯̬����������
{
	int n = (int)(process * 100.f);
	n = n > 100 ? 100 : n;
	
	const char* anim = "-\\|/";
	static int counter = 0;

	int m = 50;

	std::cout << desc1 << "|";

	for (int i = n * m / 100; i > 0; i--)
		std::cout << "��";
	for (int i = m - n * m / 100; i > 0; i--) {
		std::cout << " ";
	}
	std::cout << "|";
	printf("%2.2f %% [%c]  %s", process * 100.f , anim[counter] , desc2);//����ٷֱȽ�����
	counter = (counter + 1) % strlen(anim);
}

Console& Console::FlushLog() {
	this->cursor = getXY();
	return *this;
}

Console& Console::ClearLog() {
	COORD currXY = getXY();
	clearDisplay(this->cursor, currXY);
	moveXY(this->cursor);
	return *this;
}

Console& Console::LogProcess(const char* desc1,float process, const char* desc2) {
	loading(process, desc1, desc2);
	return *this;
}

Console& Console::LineSwitch(size_t num) {
	for(size_t i = 0;i != num;i++)
		std::cout << std::endl;
	return *this;
}

Console::Console() {
	HANDLE hStd(GetStdHandle(STD_OUTPUT_HANDLE));
	CONSOLE_CURSOR_INFO  cInfo;
	GetConsoleCursorInfo(hStd, &cInfo);			//��ȡ�����Ϣ�ľ��
	cInfo.bVisible = false;						//�޸Ĺ��ɼ���
	SetConsoleCursorInfo(hStd, &cInfo);			//���ù�겻�ɼ�

}

