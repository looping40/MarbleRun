using UnrealBuildTool;

public class MarbleRunEditorTarget : TargetRules
{
	public MarbleRunEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MarbleRun");
	}
}
