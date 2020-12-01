#ifndef TE_DEPENDENCY_MODULE_HPP
#define TE_DEPENDENCY_MODULE_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// ----------------------------------------------------------------------------
NS_DEPENDENCY

/**
* General module used to keep track of library dependencies.
*/
class TEMPORTALENGINE_API Module
{

private:
	
	/** Flag indicating if the library is currently initialized. */
	bool mIsInitialized;

protected:

	/**
	* Change the flag to indicate if the dependency has been initialized.
	* Returns the initialized flag.
	*/
	bool markAsInitialized(bool const initialized);

public:
	Module();
	virtual ~Module();

	/**
	* Querys if the module has been marked as initialized.
	*/
	bool const isInitialized() const;
	
	/**
	* Inheireted to initialize the dependency module.
	*/
	virtual bool initialize() = 0;

	/**
	* Inheireted to terminate the dependency module (default is NO-OP).
	*/
	virtual void terminate();

};

/**
* Empty dependency module with NO-OP operators.
*/
class ModuleNull : public Module
{
public:
	bool initialize() override;
};

NS_END
// ----------------------------------------------------------------------------

#endif
