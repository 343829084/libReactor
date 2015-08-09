#include "faultraise.h"

#include "../Common/Utils.h"
#include "../CFGS/ConfigStore.h"
#include "../IOS/BufferLogger.h"

namespace libReactor
{
	FaultRaise * FaultRaise::instance = NULL;

	FaultRaise::FaultRaise() :fileOutputEnabled_(true)
	{
		instance = NULL;
		faultElementQueue_ = new libReactor::LockFreeQueue<FaultElement>(MAX_NUM_OF_FAULT_ELEMENT);
		if (fileOutputEnabled_)
		{
			ostringstream strName;
			strName.clear();
			strName << ConfigStore::m_appName << "[ " << ConfigStore::m_devId << " ]" << "_log_";
			logger_ = new BufferLogger();
			logger_->Open(strName.str(), true);
		}
	}

	FaultRaise::~FaultRaise()
	{
		if (NULL != faultElementQueue_)
		{
			delete faultElementQueue_;
		}

		if (fileOutputEnabled_)
		{
			logger_->Close();
			delete logger_;
		}
	}

	void FaultRaise::Destroy()
	{
		if (NULL != faultElementQueue_)
		{
			delete faultElementQueue_;
		}
	}

	bool FaultRaise::SetFault(const bool iFaultFlag, const ErrorCode iErrorCode, const string iErrorMessage)
	{
		if (iFaultFlag)
		{
			FaultElement faultElement;
			faultElement.errorCode = iErrorCode;
			if (iErrorMessage.length() <= MAX_LENGTH_ERROR_MESSAGE)
			{
				memcpy(faultElement.errorMessage, iErrorMessage.c_str(), iErrorMessage.length());
			}
			else
			{
				string abbreviation = "...";
				memcpy(faultElement.errorMessage, iErrorMessage.substr(0, MAX_LENGTH_ERROR_MESSAGE - abbreviation.length() - 1).c_str(), MAX_LENGTH_ERROR_MESSAGE - abbreviation.length());
				strcat(faultElement.errorMessage, abbreviation.c_str());
			}
			while (!faultElementQueue_->EnQueue(faultElement))
			{
				faultElementQueue_->DeQueue(NULL);
			}
		}
		return false;
	}

	bool FaultRaise::GetLastFault(FaultElement * faultElement)
	{
		return faultElementQueue_->DeQueue(faultElement);
	}

	int FaultRaise::PrintErrors(const char * _format, const va_list _args)
	{
#if DEBUG_X

		char buffer[MAX_LENGTH_ERROR_MESSAGE + 1];
		int len = vsnprintf(buffer, MAX_LENGTH_ERROR_MESSAGE, _format, _args);
		buffer[len] = '\0';

		if (fileOutputEnabled_)
		{
			logger_->WirteImmediately(buffer, len);
		}
		return vprintf(_format, _args);
#else
		return 0;
#endif

	}

	int FaultRaise::PrintErrors(const char * _format...)
	{
#if DEBUG_X
		va_list args;
		va_start(args, _format);
		/*
		char buffer[MAX_LENGTH_ERROR_MESSAGE+1];
		int len = vsnprintf( buffer, MAX_LENGTH_ERROR_MESSAGE, _format, _args );
		buffer[len] = '\0';
		*/
		int len = vprintf(_format, args);
		va_end(args);
		return len;
#else
		return 0;
#endif
	}

	FaultRaise * FaultRaise::Instance()
	{
		if (NULL == instance)
		{
			instance = new FaultRaise();
		}
		return instance;
	}
}
