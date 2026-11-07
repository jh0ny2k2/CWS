#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

/**
 * Categorias de log del proyecto (F0.06).
 *
 * Regla: no usar el log temporal generico en codigo de juego. Cada sistema escribe en su categoria
 * para poder filtrar en la consola (`Log LogCWVehicle VeryVerbose`) y en los logs.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogCW, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogCWInput, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogCWVehicle, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogCWInteraction, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogCWPerf, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogCWAssets, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogCWTest, Log, All);
