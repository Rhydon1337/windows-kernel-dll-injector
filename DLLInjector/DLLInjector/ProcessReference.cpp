#include "ProcessReference.h"

ProcessReference::ProcessReference()
	: m_process(nullptr) {
}

ProcessReference::~ProcessReference() {
	if (nullptr != m_process) {
		ObDereferenceObject(m_process);
		if (m_attach) {
			KeUnstackDetachProcess(&m_apc_state);
		}
	}
	
}

NTSTATUS ProcessReference::init(size_t pid, bool attach) {
	CHECK(PsLookupProcessByProcessId(reinterpret_cast<HANDLE>(pid), &m_process));
	m_attach = attach;
	if (attach) {
		KeStackAttachProcess(m_process, &m_apc_state);
	}
	return STATUS_SUCCESS;
}
