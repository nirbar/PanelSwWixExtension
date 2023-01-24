using PanelSw.Wix.Extensions.Symbols;
using System.Reflection;
using WixToolset.Data;
using WixToolset.Extensibility;

namespace PanelSw.Wix.Extensions
{
    internal class PanelSwWixExtData : BaseExtensionData
    {
        public override string DefaultCulture => "en-US";

        public override bool TryGetSymbolDefinitionByName(string name, out IntermediateSymbolDefinition symbolDefinition)
        {
            switch (name)
            {
                case nameof(PSW_AccountSidSearch):
                    symbolDefinition = PSW_AccountSidSearch.SymbolDefinition;
                    break;
                case nameof(PSW_BackupAndRestore):
                    symbolDefinition = PSW_BackupAndRestore.SymbolDefinition;
                    break;
                case nameof(PSW_CertificateHashSearch):
                    symbolDefinition = PSW_CertificateHashSearch.SymbolDefinition;
                    break;
                case nameof(PSW_ConcatFiles):
                    symbolDefinition = PSW_ConcatFiles.SymbolDefinition;
                    break;
                case nameof(PSW_ContainerTemplate):
                    symbolDefinition = PSW_ContainerTemplate.SymbolDefinition;
                    break;
                case nameof(PSW_CustomUninstallKey):
                    symbolDefinition = PSW_CustomUninstallKey.SymbolDefinition;
                    break;
                case nameof(PSW_DeletePath):
                    symbolDefinition = PSW_DeletePath.SymbolDefinition;
                    break;
                case nameof(PSW_DiskSpace):
                    symbolDefinition = PSW_DiskSpace.SymbolDefinition;
                    break;
                case nameof(PSW_Dism):
                    symbolDefinition = PSW_Dism.SymbolDefinition;
                    break;
                case nameof(PSW_EvaluateExpression):
                    symbolDefinition = PSW_EvaluateExpression.SymbolDefinition;
                    break;
                case nameof(PSW_ExecOn_ConsoleOutput):
                    symbolDefinition = PSW_ExecOn_ConsoleOutput.SymbolDefinition;
                    break;
                case nameof(PSW_ExecOnComponent):
                    symbolDefinition = PSW_ExecOnComponent.SymbolDefinition;
                    break;
                case nameof(PSW_ExecOnComponent_Environment):
                    symbolDefinition = PSW_ExecOnComponent_Environment.SymbolDefinition;
                    break;
                case nameof(PSW_ExecOnComponent_ExitCode):
                    symbolDefinition = PSW_ExecOnComponent_ExitCode.SymbolDefinition;
                    break;
                case nameof(PSW_FileRegex):
                    symbolDefinition = PSW_FileRegex.SymbolDefinition;
                    break;
                case nameof(PSW_ForceVersion):
                    symbolDefinition = PSW_ForceVersion.SymbolDefinition;
                    break;
                case nameof(PSW_InstallUtil):
                    symbolDefinition = PSW_InstallUtil.SymbolDefinition;
                    break;
                case nameof(PSW_InstallUtil_Arg):
                    symbolDefinition = PSW_InstallUtil_Arg.SymbolDefinition;
                    break;
                case nameof(PSW_JsonJPath):
                    symbolDefinition = PSW_JsonJPath.SymbolDefinition;
                    break;
                case nameof(PSW_JsonJpathSearch):
                    symbolDefinition = PSW_JsonJpathSearch.SymbolDefinition;
                    break;
                case nameof(PSW_Md5Hash):
                    symbolDefinition = PSW_Md5Hash.SymbolDefinition;
                    break;
                case nameof(PSW_MsiSqlQuery):
                    symbolDefinition = PSW_MsiSqlQuery.SymbolDefinition;
                    break;
                case nameof(PSW_PathSearch):
                    symbolDefinition = PSW_PathSearch.SymbolDefinition;
                    break;
                case nameof(PSW_Payload):
                    symbolDefinition = PSW_Payload.SymbolDefinition;
                    break;
                case nameof(PSW_ReadIniValues):
                    symbolDefinition = PSW_ReadIniValues.SymbolDefinition;
                    break;
                case nameof(PSW_RegularExpression):
                    symbolDefinition = PSW_RegularExpression.SymbolDefinition;
                    break;
                case nameof(PSW_RemoveRegistryValue):
                    symbolDefinition = PSW_RemoveRegistryValue.SymbolDefinition;
                    break;
                case nameof(PSW_RestartLocalResources):
                    symbolDefinition = PSW_RestartLocalResources.SymbolDefinition;
                    break;
                case nameof(PSW_SelfSignCertificate):
                    symbolDefinition = PSW_SelfSignCertificate.SymbolDefinition;
                    break;
                case nameof(PSW_ServiceConfig):
                    symbolDefinition = PSW_ServiceConfig.SymbolDefinition;
                    break;
                case nameof(PSW_ServiceConfig_Dependency):
                    symbolDefinition = PSW_ServiceConfig_Dependency.SymbolDefinition;
                    break;
                case nameof(PSW_SetPropertyFromPipe):
                    symbolDefinition = PSW_SetPropertyFromPipe.SymbolDefinition;
                    break;
                case nameof(PSW_ShellExecute):
                    symbolDefinition = PSW_ShellExecute.SymbolDefinition;
                    break;
                case nameof(PSW_SqlScript):
                    symbolDefinition = PSW_SqlScript.SymbolDefinition;
                    break;
                case nameof(PSW_SqlScript_Replacements):
                    symbolDefinition = PSW_SqlScript_Replacements.SymbolDefinition;
                    break;
                case nameof(PSW_SqlSearch):
                    symbolDefinition = PSW_SqlSearch.SymbolDefinition;
                    break;
                case nameof(PSW_TaskScheduler):
                    symbolDefinition = PSW_TaskScheduler.SymbolDefinition;
                    break;
                case nameof(PSW_Telemetry):
                    symbolDefinition = PSW_Telemetry.SymbolDefinition;
                    break;
                case nameof(PSW_ToLowerCase):
                    symbolDefinition = PSW_ToLowerCase.SymbolDefinition;
                    break;
                case nameof(PSW_TopShelf):
                    symbolDefinition = PSW_TopShelf.SymbolDefinition;
                    break;
                case nameof(PSW_Unzip):
                    symbolDefinition = PSW_Unzip.SymbolDefinition;
                    break;
                case nameof(PSW_VersionCompare):
                    symbolDefinition = PSW_VersionCompare.SymbolDefinition;
                    break;
                case nameof(PSW_WebsiteConfig):
                    symbolDefinition = PSW_WebsiteConfig.SymbolDefinition;
                    break;
                case nameof(PSW_WmiSearch):
                    symbolDefinition = PSW_WmiSearch.SymbolDefinition;
                    break;
                case nameof(PSW_XmlSearch):
                    symbolDefinition = PSW_XmlSearch.SymbolDefinition;
                    break;
                case nameof(PSW_XslTransform):
                    symbolDefinition = PSW_XslTransform.SymbolDefinition;
                    break;
                case nameof(PSW_XslTransform_Replacements):
                    symbolDefinition = PSW_XslTransform_Replacements.SymbolDefinition;
                    break;
                case nameof(PSW_ZipFile):
                    symbolDefinition = PSW_ZipFile.SymbolDefinition;
                    break;
                default:
                    symbolDefinition = null;
                    break;
            }

            return symbolDefinition != null;
        }

        public override Intermediate GetLibrary(ISymbolDefinitionCreator symbolDefinitions)
        {
            Assembly me = Assembly.GetExecutingAssembly();
            return Intermediate.Load(me, "PanelSwWixLib.wixlib", symbolDefinitions);
        }
    }
}
