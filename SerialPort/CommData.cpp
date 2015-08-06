#include "CommData.h"


/*--------------------------------------------------------------------------
FUNCTION NAME: EraseMultiSpace
DESCRIPTION: ��ȥ��β�ո�ϲ�����ո�
AUTHOR�������� 
PARAMETERS: str
RETURN: 
*-------------------------------------------------------------------------*/
void EraseMultiSpace(std::string &str)
{
	bool alnumFlag = false;
	bool spaceFlag = false;

	int j=0;
	for (int i=0; i<str.length(); )
	{
		if(!isspace(str.at(i)))
		{	
			str.at(j) = str.at(i);
			j++;
			alnumFlag = true;
			spaceFlag = false;
		}	
		else
		{
			spaceFlag = true;
			if(alnumFlag)
			{
				str.at(j) = str.at(i);
				j++;
				alnumFlag = false;
			}
		}
		i++;
	}

	if (spaceFlag)
	{
		str.erase(j-1);
	}
	else
	{
		str.erase(j);
	}
} 


/*--------------------------------------------------------------------------
FUNCTION NAME: getDigit
DESCRIPTION: �������ַ����е�����
AUTHOR�������� 
PARAMETERS: dataStr1 ��������com1������1
RETURN: 
*-------------------------------------------------------------------------*/
int getDigit(std::string &dataStr1)
{
	//���������ں�
	int beginDigit = 0;
	int endDigit = 0;
	bool digitFlag = false;
	for (int i=0; i<dataStr1.length(); i++)
	{
		if(isdigit(dataStr1.at(i)))
		{	
			if (digitFlag == false)
			{
				beginDigit = i;
				digitFlag = true;
			}
			endDigit = i;
		}
	}
	dataStr1 = dataStr1.substr(beginDigit, endDigit);
	return atoi(dataStr1.c_str());
}

/*--------------------------------------------------------------------------
FUNCTION NAME: AnalyseCmdBuf
DESCRIPTION: �������ַ����е�port chanel chanelStatus
AUTHOR�������� 
PARAMETERS: IN recvStr, INOUT port, INOUT chanel, INOUT chanelStatus 
            ��������com1 chanel1 ON�� port=1�� chanel=1�� chanelStatus=1
RETURN: 
*-------------------------------------------------------------------------*/
bool AnalyseCmdBuf(std::string recvStr, int & port, int & chanel, int & chanelStatus)
{
	EraseMultiSpace(recvStr);

	int posFirst = recvStr.find_first_of(" ");
	std::string dataStr1 = recvStr.substr(0, posFirst);
	port = getDigit(dataStr1);
	if (port < 1 || port >16)
	{
		return false;
	}

	int posLast = recvStr.find_last_of(" ");
	dataStr1 = recvStr.substr(posFirst+1, posLast-posFirst-1);
	chanel = getDigit(dataStr1);
	if (chanel < 1 || chanel >16)
	{
		return false;
	}

	dataStr1 = recvStr.substr(posLast+1, std::string::npos-posLast-1);
	transform(dataStr1.begin(), dataStr1.end(), dataStr1.begin(), ::tolower);
	if (!dataStr1.compare("on"))
	{
		chanelStatus = 1;
	}
	else if (!dataStr1.compare("off"))
	{
		chanelStatus = 0;
	}
	else
	{
		return false;
	}

	return true;
}