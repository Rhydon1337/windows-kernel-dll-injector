#pragma once

#include <ntifs.h>

#include "consts.h"

class ProcessReference {
public:
	ProcessReference();
	~ProcessReference();

	NTSTATUS init(size_t pid, bool attach);
	
	DELETE_DEFAULT_CTORS(ProcessReference);
private:
	PEPROCESS m_process;
	bool m_attach{};
	KAPC_STATE* m_apc_state;
};

