using UnrealBuildTool;

public class MarbleRunTarget : TargetRules
{
	public MarbleRunTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MarbleRun");
	}
}
