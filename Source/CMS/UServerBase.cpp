/*
*********************************************************************************************************
*
*                                     COMMON TASK AND SEMAPHORE
* 
* Project       : libReactor
* Filename      : UServerBase.cpp
* Version       : V1.0
* Programmer(s) : xclyfe@gmail.com
*********************************************************************************************************
*/
/*
*********************************************************************************************************
*                                        INCLUDE FILES
*********************************************************************************************************
*/
#include "UServerBase.h"
#include "../IFS/ReflectionX.h"

UServerBase::UServerBase(UINT16 bufferSize, UINT16 numDataReader, UINT16 unmDataProcesser, long timeOut):
	serverAccepter_(NULL),vecUDataReader_(NULL),vecDataProcesser_(NULL),vecDataHandler_(NULL),UBufferContainer_(NULL),
	ServiceStarted_(false),
	bufferSize_(bufferSize),numDataReader_(numDataReader),numDataProcesser_(unmDataProcesser),timeOut_(timeOut)
{
}

UServerBase::~UServerBase(void)
{
	Destroy();
}

bool  UServerBase::SetParameters(UINT16 bufferSize, UINT16 numDataReader, UINT16 numDataProcesser, long timeOut)
{
	if(ServiceStarted_)
	{
		return false;
	}

	this->bufferSize_ = bufferSize;
	this->numDataReader_ = numDataReader;
	this->numDataProcesser_ = numDataProcesser;
	this->timeOut_ = timeOut;

	return true;
}

void UServerBase::Execute(string serverHost,       //Local host location
						  int serverPort,              //Local host port
						  string dataHandlerClassName,
						  long timeOut)
{
	if(ServiceStarted_)
	{
		return;
	}

	//Create the communication server thread
	serverAccepter_ = new UServerAccepter();
	serverAccepter_->Initialize(serverHost,serverPort);
	serverAccepter_->SetTimeOut(timeOut);
	serverAccepter_->Start();

	//create the data process thread list
	UBufferContainer_ = new UBufferContainer(bufferSize_);

	for (int i = 0; i < numDataReader_; i++)
	{
		UDataReader * dataReader = new UDataReader(serverAccepter_);
		dataReader->SetBufferListRev(UBufferContainer_);
		dataReader->Start();

		vecUDataReader_.push_back(dataReader);
	}

	for (int i = 0; i < numDataProcesser_; i++)
	{
		UDataProcesser * dataProcesser = new UDataProcesser(UBufferContainer_);

		//create the data handler for dataProcesser
		UServerDataHandler * dataHandler = (UServerDataHandler *)ReflectionClassFactory::GetClassByName(dataHandlerClassName);
		dataHandler->SetServerAccepter(serverAccepter_);
		dataProcesser->SetDataHanler(dataHandler);
		dataProcesser->Start();

		vecDataHandler_.push_back(dataHandler);
		vecDataProcesser_.push_back(dataProcesser);
	}

	ServiceStarted_ = true;
}

bool UServerBase::HandleOutput(UdpBuffer & udpBuffer)
{
	bool result = false;
	if(ServiceStarted_ && NULL != serverAccepter_)
	{
		result = serverAccepter_->HandleOutput(udpBuffer);
	}
	return result;
}


void UServerBase::WaitForExit()
{
	this->serverAccepter_->WaitForExit();
}

void UServerBase::Destroy()
{
	if(!ServiceStarted_)
	{
		return;
	}

	//serverAccepter_
	if(NULL != serverAccepter_)
	{
		serverAccepter_->SetEnable(false);
		serverAccepter_->WaitForExit();
	}

	//vecUDataReader_
	vector<UDataReader *>::iterator j = vecUDataReader_.begin();
	for (; j != vecUDataReader_.end(); j++)
	{
		(*j)->SetEnable(false);
		(*j)->TerminateThreadX(0);
		delete (*j);
	}

	//vecDataProcesser_
	vector<UDataProcesser *>::iterator i = vecDataProcesser_.begin();
	for (; i != vecDataProcesser_.end(); i++)
	{
		(*i)->SetEnable(false);
		(*i)->TerminateThreadX(0);
		delete (*i);
	}

	//vecDataHandler_
	vector<UServerDataHandler *>::iterator k = vecDataHandler_.begin();
	for (; k != vecDataHandler_.end(); k++)
	{
		delete (*i);
	}

	//UBufferContainer_
	if(NULL != UBufferContainer_)
	{
		delete UBufferContainer_;
		UBufferContainer_ = NULL;
	}

	if(NULL != serverAccepter_)
	{
		delete serverAccepter_;
		serverAccepter_ = NULL;
	}

	ServiceStarted_ = false;
}
