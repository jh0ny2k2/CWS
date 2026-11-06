using UnrealBuildTool;
using System.Collections.Generic;

public class ConstructionWorldEditorTarget : TargetRules
{
	public ConstructionWorldEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("ConstructionWorld");
	}
}
