#include "CWGameMode.h"

#include "CWHUD.h"
#include "CWPlayerCharacter.h"

ACWGameMode::ACWGameMode()
{
	DefaultPawnClass = ACWPlayerCharacter::StaticClass();
	HUDClass = ACWHUD::StaticClass();
}
