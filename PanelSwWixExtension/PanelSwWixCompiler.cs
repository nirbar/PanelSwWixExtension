using WixToolset.Extensibility;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Xml;
using System.Xml.Schema;
using System.Xml.Linq;
using System.Runtime.CompilerServices;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;
using WixToolset.Data.Symbols;
using PanelSw.Wix.Extensions.Symbols;

namespace PanelSw.Wix.Extensions
{
    /// <summary>
    /// The compiler for the Windows Installer XML Toolset PanelSwWixExtension Extension.
    /// </summary>
    public sealed class PanelSwWixCompiler : BaseCompilerExtension
    {
        public override XNamespace Namespace => XNamespace.Get("http://schemas.panel-sw.co.il/wix/WixExtension");

        /// <summary>
        /// Processes an element for the Compiler.
        /// </summary>
        /// <param name="sourceLineNumbers">Source line number for the parent element.</param>
        /// <param name="parentElement">Parent element of element to process.</param>
        /// <param name="element">Element to process.</param>
        /// <param name="contextValues">Extra information about the context in which this element is being parsed.</param>
        public override void ParseElement(Intermediate intermediate, IntermediateSection section, XElement parentElement, XElement element, IDictionary<string, string> context)
        {
            switch (parentElement.Name.LocalName)
            {
                case "Fragment":
                case "Module":
                case "Product":
                    switch (element.Name.LocalName)
                    {
                        case "CustomUninstallKey":
                            ParseCustomUninstallKeyElement(section, element);
                            break;

                        case "Payload":
                            ParsePayload(section, element, null, null);
                            break;

                        case "FileRegex":
                            ParseFileRegex(section, element, null, null);
                            break;

                        case "ReadIniValues":
                            ParseReadIniValuesElement(section, element, parentElement);
                            break;

                        case "RemoveRegistryValue":
                            ParseRemoveRegistryValue(section, element);
                            break;

                        case "Telemetry":
                            ParseTelemetry(section, element);
                            break;

                        case "ShellExecute":
                            ParseShellExecute(section, element);
                            break;

                        case "MsiSqlQuery":
                            ParseMsiSqlQuery(section, element, parentElement);
                            break;

                        case "RegularExpression":
                            ParseRegularExpression(section, element);
                            break;

                        case "RestartLocalResources":
                            ParseRestartLocalResources(section, element);
                            break;

                        case "DeletePath":
                            ParseDeletePath(section, element);
                            break;

                        case "ZipFile":
                            ParseZipFile(section, element);
                            break;

                        case "Unzip":
                            ParseUnzip(section, element);
                            break;

                        case "SetPropertyFromPipe":
                            ParseSetPropertyFromPipe(section, element);
                            break;

                        default:
                            ParseHelper.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                case "DirectoryRef":
                case "Directory":
                    {
                        string directoryId = context["DirectoryId"];

                        switch (element.Name.LocalName)
                        {
                            case "DiskSpace":
                                ParseDiskSpaceElement(section, element, directoryId);
                                break;

                            default:
                                ParseHelper.UnexpectedElement(parentElement, element);
                                break;
                        }
                        break;
                    }

                case "Property":
                    switch (element.Name.LocalName)
                    {
                        case "AccountSidSearch":
                            ParseAccountSidSearchElement(section, element);
                            break;

                        case "PathSearch":
                            ParsePathSearchElement(section, element);
                            break;

                        case "VersionCompare":
                            ParseVersionCompareElement(section, element);
                            break;

                        case "XmlSearch":
                            ParseXmlSearchElement(section, element);
                            break;

                        case "WmiSearch":
                            ParseWmiSearchElement(section, element);
                            break;

                        case "SqlSearch":
                            ParseSqlSearchElement(section, element);
                            break;

                        case "Evaluate":
                            ParseEvaluateElement(section, element);
                            break;

                        case "CertificateHashSearch":
                            ParseCertificateHashSearchElement(section, element);
                            break;

                        case "JsonJpathSearch":
                            ParseJsonJpathSearchElement(section, element);
                            break;

                        case "MsiSqlQuery":
                            ParseMsiSqlQuery(section, element, parentElement);
                            break;

                        case "ReadIniValues":
                            ParseReadIniValuesElement(section, element, parentElement);
                            break;

                        case "RegularExpression":
                            ParseRegularExpression(section, element);
                            break;

                        case "ToLowerCase":
                            ParseToLowerCase(section, element);
                            break;

                        case "Md5Hash":
                            ParseMd5Hash(section, element);
                            break;

                        default:
                            ParseHelper.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                case "Component":
                    {
                        string componentId = context["ComponentId"];
                        string directoryId = context["DirectoryId"];
                        bool isWin64 = bool.Parse(context["Win64"]);

                        switch (element.Name.LocalName)
                        {
                            case "JsonJPath":
                                ParseJsonJPathElement(section, element, componentId, null);
                                break;

                            case "TaskScheduler":
                                ParseTaskSchedulerElement(section, element, componentId);
                                break;

                            case "ExecOn":
                            case "ExecOnComponent":
                                ParseExecOnComponentElement(section, element, componentId);
                                break;

                            case "Dism":
                                ParseDismElement(section, element, componentId);
                                break;

                            case "ServiceConfig":
                                ParseServiceConfigElement(section, element, componentId);
                                break;

                            case "BackupAndRestore":
                                ParseBackupAndRestoreElement(section, element, componentId);
                                break;

                            case "CreateSelfSignCertificate":
                                ParseCreateSelfSignCertificateElement(section, element, componentId);
                                break;

                            case "SqlScript":
                                ParseSqlScriptElement(section, element, componentId);
                                break;

                            case "WebsiteConfig":
                                ParseWebsiteConfigElement(section, element, componentId);
                                break;

                            case "XslTransform":
                                ParseXslTransform(section, element, componentId, null);
                                break;

                            default:
                                ParseHelper.UnexpectedElement(parentElement, element);
                                break;
                        }
                        break;
                    }

                case "File":
                    {
                        string fileId = context["FileId"];
                        string componentId = context["ComponentId"];
                        bool isWin64 = bool.Parse(context["Win64"]);
                        string directoryId = context["DirectoryId"];

                        switch (element.Name.LocalName)
                        {
                            case "JsonJPath":
                                ParseJsonJPathElement(section, element, componentId, fileId);
                                break;

                            case "InstallUtil":
                                ParseInstallUtilElement(section, element, fileId);
                                break;

                            case "TopShelf":
                                ParseTopShelfElement(section, element, fileId);
                                break;

                            case "AlwaysOverwriteFile":
                            case "ForceVersion":
                                ParseForceVersionElement(section, element, fileId);
                                break;

                            case "FileRegex":
                                ParseFileRegex(section, element, componentId, fileId);
                                break;

                            case "XslTransform":
                                ParseXslTransform(section, element, componentId, fileId);
                                break;

                            case "SplitFile":
                                ParseSplitFileElement(section, parentElement, element, componentId, fileId);
                                break;

                            default:
                                ParseHelper.UnexpectedElement(parentElement, element);
                                break;
                        }
                        break;
                    }

                case "PatchFamily":
                    switch (element.Name.LocalName)
                    {
                        case "CustomPatchRef":
                            ParseCustomPatchRefElement(section, element);
                            break;
                        default:
                            ParseHelper.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                default:
                    ParseHelper.UnexpectedElement(parentElement, element);
                    break;
            }
        }

        private void ParsePayload(IntermediateSection section, XElement element, object p1, object p2)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string source = null;
            string name = null;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Source":
                            source = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Name":
                            name = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(source))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Source"));
                return;
            }
            if (string.IsNullOrEmpty(name))
            {
                name = Path.GetFileName(source);
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ExtractPayload");

            string binaryKey = $"pld{Guid.NewGuid().ToString("N")}";
            BinarySymbol binaryRow = section.AddSymbol(new BinarySymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, binaryKey)));
            binaryRow.Data = new IntermediateFieldPathValue() { Path = source };

            PSW_Payload pldRow = section.AddSymbol(new PSW_Payload(sourceLineNumbers, binaryKey));
            pldRow.Name = name;
        }
        
        private void ParseSplitFileElement(IntermediateSection section, XElement fileElement, XElement element, string componentId, string fileId)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            int splitSize = Int32.MaxValue; //2GB

            foreach (XAttribute attrib in element.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Size":
                            splitSize = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, 1, Int32.MaxValue);
                            break;
                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            XAttribute sourceAttrib = fileElement.Attribute("Source");
            if (sourceAttrib == null)
            {
                sourceAttrib = fileElement.Attribute("src");
            }
            string sourcePath = ParseHelper.GetAttributeValue(sourceLineNumbers, sourceAttrib);
            if (!File.Exists(sourcePath))
            {
                Messaging.Write(ErrorMessages.FileNotFound(sourceLineNumbers, sourcePath));
                return;
            }
            FileInfo fileInfo = new FileInfo(sourcePath);
            if (fileInfo.Length <= splitSize)
            {
                return;
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ConcatFiles");

            string tmpPath = Path.GetTempPath();
            int splitCnt = (int)Math.Ceiling(1m * fileInfo.Length / splitSize);
            for (int i = 1; i < splitCnt; ++i)
            {
                XElement splitFileElement = new XElement(fileElement);
                string splId = "spl" + Guid.NewGuid().ToString("N");
                string splFile = Path.Combine(tmpPath, splId);
                File.Create(splFile).Dispose();

                splitFileElement.SetAttributeValue("KeyPath", "no");
                splitFileElement.SetAttributeValue("CompanionFile", fileId);
                splitFileElement.SetAttributeValue("Name", splId);
                splitFileElement.SetAttributeValue("Id", splId);
                splitFileElement.SetAttributeValue("Source", splFile);

                fileElement.Parent.Add(splitFileElement);

                if (!Messaging.EncounteredError)
                {
                    PSW_ConcatFiles row = section.AddSymbol(new PSW_ConcatFiles(sourceLineNumbers));
                    row.Component_ = componentId;
                    row.RootFile_ = fileId;
                    row.MyFile_ = splId;
                    row.Order = i;
                    row.Size = splitSize;
                }
            }
        }

        private void ParseCustomPatchRefElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string table = null;
            string key = null;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Table":
                            table = ParseHelper.GetAttributeIdentifierValue(sourceLineNumbers, attrib);
                            break;
                        case "Key":
                            key = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(table))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Table"));
            }
            if (string.IsNullOrEmpty(key))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Key"));
            }

            // When referencing a table row, reference the relevant custom actions
            List<string> customActions = new List<string>();
            switch (table)
            {
                case "PSW_XmlSearch":
                    customActions.Add("XmlSearch");
                    break;
                case "PSW_FileRegex":
                    customActions.Add("FileRegex_Immediate");
                    customActions.Add("FileRegex_rollback");
                    customActions.Add("FileRegex_deferred");
                    customActions.Add("FileRegex_commit");
                    break;
                case "PSW_CustomUninstallKey":
                    customActions.Add("CustomUninstallKey_Immediate");
                    customActions.Add("CustomUninstallKey_deferred");
                    customActions.Add("CustomUninstallKey_rollback");
                    break;
                case "PSW_ReadIniValues":
                    customActions.Add("ReadIniValues");
                    break;
                case "PSW_RemoveRegistryValue":
                    customActions.Add("RemoveRegistryValue_Immediate");
                    customActions.Add("RemoveRegistryValue_deferred");
                    customActions.Add("RemoveRegistryValue_rollback");
                    break;
                case "PSW_RegularExpression":
                    customActions.Add("RegularExpression");
                    break;
                case "PSW_Telemetry":
                    customActions.Add("Telemetry");
                    customActions.Add("Telemetry_deferred");
                    customActions.Add("Telemetry_rollback");
                    customActions.Add("Telemetry_commit");
                    break;
                case "PSW_ShellExecute":
                    customActions.Add("ShellExecute_Immediate");
                    customActions.Add("ShellExecute_deferred");
                    customActions.Add("ShellExecute_rollback");
                    customActions.Add("ShellExecute_commit");
                    break;
                case "PSW_MsiSqlQuery":
                    customActions.Add("MsiSqlQuery");
                    break;
                case "PSW_DeletePath":
                    customActions.Add("DeletePath");
                    customActions.Add("DeletePath_rollback");
                    customActions.Add("DeletePath_deferred");
                    customActions.Add("DeletePath_commit");
                    break;
                case "PSW_TaskScheduler":
                    customActions.Add("TaskScheduler");
                    customActions.Add("TaskScheduler_rollback");
                    customActions.Add("TaskScheduler_deferred");
                    customActions.Add("TaskScheduler_commit");
                    break;
                case "PSW_ExecOnComponent":
                case "PSW_ExecOnComponent_ExitCode":
                case "PSW_ExecOn_ConsoleOutput":
                case "PSW_ExecOnComponent_Environment":
                    customActions.Add("ExecOnComponent");
                    customActions.Add("ExecOnComponentRollback");
                    customActions.Add("ExecOnComponentCommit");
                    customActions.Add("ExecOnComponent_BeforeStop_rollback");
                    customActions.Add("ExecOnComponent_BeforeStop_deferred");
                    customActions.Add("ExecOnComponent_AfterStop_rollback");
                    customActions.Add("ExecOnComponent_AfterStop_deferred");
                    customActions.Add("ExecOnComponent_BeforeStart_rollback");
                    customActions.Add("ExecOnComponent_BeforeStart_deferred");
                    customActions.Add("ExecOnComponent_AfterStart_rollback");
                    customActions.Add("ExecOnComponent_AfterStart_deferred");
                    customActions.Add("ExecOnComponent_Imp_BeforeStop_rollback");
                    customActions.Add("ExecOnComponent_Imp_BeforeStop_deferred");
                    customActions.Add("ExecOnComponent_Imp_AfterStop_rollback");
                    customActions.Add("ExecOnComponent_Imp_AfterStop_deferred");
                    customActions.Add("ExecOnComponent_Imp_BeforeStart_rollback");
                    customActions.Add("ExecOnComponent_Imp_BeforeStart_deferred");
                    customActions.Add("ExecOnComponent_Imp_AfterStart_rollback");
                    customActions.Add("ExecOnComponent_Imp_AfterStart_deferred");
                    break;
                case "PSW_Dism":
                    customActions.Add("DismSched");
                    customActions.Add("DismX86");
                    customActions.Add("DismX64");
                    break;
                case "PSW_ZipFile":
                    customActions.Add("ZipFileSched");
                    customActions.Add("ZipFileExec");
                    break;
                case "PSW_Unzip":
                    customActions.Add("UnzipSched");
                    customActions.Add("UnzipExec");
                    break;
                case "PSW_ServiceConfig":
                case "PSW_ServiceConfig_Dependency":
                    customActions.Add("PSW_ServiceConfig");
                    customActions.Add("PSW_ServiceConfigRlbk");
                    customActions.Add("PSW_ServiceConfigExec");
                    break;
                case "PSW_InstallUtil":
                case "PSW_InstallUtil_Arg":
                    customActions.Add("PSW_InstallUtilSched");
                    customActions.Add("PSW_InstallUtil_InstallExec_x86");
                    customActions.Add("PSW_InstallUtil_InstallRollback_x86");
                    customActions.Add("PSW_InstallUtil_UninstallExec_x86");
                    customActions.Add("PSW_InstallUtil_UninstallRollback_x86");
                    customActions.Add("PSW_InstallUtil_InstallExec_x64");
                    customActions.Add("PSW_InstallUtil_InstallRollback_x64");
                    customActions.Add("PSW_InstallUtil_UninstallExec_x64");
                    customActions.Add("PSW_InstallUtil_UninstallRollback_x64");
                    break;
                case "PSW_SqlSearch":
                    customActions.Add("SqlSearch");
                    break;
                case "PSW_BackupAndRestore":
                    customActions.Add("BackupAndRestore");
                    customActions.Add("BackupAndRestore_rollback");
                    customActions.Add("BackupAndRestore_deferred");
                    customActions.Add("BackupAndRestore_commit");
                    break;
                case "PSW_TopShelf":
                    customActions.Add("TopShelf");
                    customActions.Add("TopShelfService_InstallRollback");
                    customActions.Add("TopShelfService_Install");
                    customActions.Add("TopShelfService_UninstallRollback");
                    customActions.Add("TopShelfService_Uninstall");
                    break;
                case "PSW_SelfSignCertificate":
                    customActions.Add("CreateSelfSignCertificate");
                    customActions.Add("CreateSelfSignCertificate_commit");
                    break;
                case "PSW_ForceVersion":
                    customActions.Add("ForceVersion");
                    break;
                case "PSW_SetPropertyFromPipe":
                    customActions.Add("SetPropertyFromPipe");
                    break;
                case "PSW_EvaluateExpression":
                    customActions.Add("EvaluateExpression");
                    break;
                case "PSW_CertificateHashSearch":
                    customActions.Add("CertificateHashSearch");
                    break;
                case "PSW_DiskSpace":
                    customActions.Add("DiskSpace");
                    break;
                case "PSW_JsonJPath":
                    customActions.Add("JsonJpathSched");
                    customActions.Add("JsonJpathExec");
                    break;
                case "PSW_JsonJpathSearch":
                    customActions.Add("JsonJpathSearch");
                    break;
                case "PSW_AccountSidSearch":
                    customActions.Add("AccountSidSearch");
                    break;
                case "PSW_XslTransform":
                case "PSW_XslTransform_Replacements":
                    customActions.Add("PSW_XslTransform");
                    customActions.Add("PSW_XslTransformExec");
                    break;
                case "PSW_SqlScript":
                case "PSW_SqlScript_Replacements":
                    customActions.Add("PSW_SqlScript");
                    customActions.Add("PSW_SqlScriptRollback");
                    customActions.Add("PSW_SqlScriptExec");
                    break;
                case "PSW_WebsiteConfig":
                    customActions.Add("PSW_WebsiteConfigSched");
                    customActions.Add("PSW_WebsiteConfigExec");
                    break;
                case "PSW_VersionCompare":
                    customActions.Add("PSW_VersionCompare");
                    break;
                case "PSW_PathSearch":
                    customActions.Add("PSW_PathSearch");
                    break;
                case "PSW_ToLowerCase":
                    customActions.Add("PSW_ToLowerCase");
                    break;
                case "PSW_WmiSearch":
                    customActions.Add("WmiSearch");
                    break;
                case "PSW_RestartLocalResources":
                    customActions.Add("RestartLocalResources");
                    customActions.Add("RestartLocalResourcesExec"); break;
                case "PSW_Md5Hash":
                    customActions.Add("Md5Hash");
                    break;
                case "PSW_ConcatFiles":
                    customActions.Add("ConcatFiles");
                    customActions.Add("ConcatFilesExec");
                    break;
                case "PSW_Payload":
                    customActions.Add("ExtractPayload");
                    customActions.Add("ExtractPayloadRollback");
                    customActions.Add("ExtractPayloadCommit");
                    break;
            }

            if (!Messaging.EncounteredError)
            {
                WixPatchRefSymbol patchReferenceRow;
                foreach (string ca in customActions)
                {
                    patchReferenceRow = section.AddSymbol(new WixPatchRefSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, "CustomAction")));
                    patchReferenceRow.Table = "CustomAction";
                    patchReferenceRow.PrimaryKeys = ca;
                }

                patchReferenceRow = section.AddSymbol(new WixPatchRefSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, "Binary")));
                patchReferenceRow.Table = "Binary";
                patchReferenceRow.PrimaryKeys = "PanelSwCustomActions.dll";

                patchReferenceRow = section.AddSymbol(new WixPatchRefSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, table)));
                patchReferenceRow.Table = table;
                patchReferenceRow.PrimaryKeys = key;
            }
        }

        public override void ParseAttribute(Intermediate intermediate, IntermediateSection section, XElement parentElement, XAttribute attribute, IDictionary<string, string> context)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(parentElement);
            if (!parentElement.Name.Namespace.Equals(Namespace))
            {
                ParseHelper.UnexpectedAttribute(parentElement, attribute);
                return;
            }

            switch (parentElement.Name.LocalName)
            {
                case "CustomAction":
                    switch (attribute.Name.LocalName)
                    {
                        case "CustomActionData":
                            ParseCustomActionDataAttribute(section, parentElement, attribute);
                            break;

                        case "ActionStartText":
                            ParseActionStartTextAttribute(section, parentElement, attribute);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(parentElement, attribute);
                            break;
                    }
                    break;

                default:
                    ParseHelper.UnexpectedAttribute(parentElement, attribute);
                    break;
            }
        }

        private void ParseActionStartTextAttribute(IntermediateSection section, XElement parentElement, XAttribute attribute)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(parentElement);
            string actionStartText = ParseHelper.GetAttributeValue(sourceLineNumbers, attribute);
            if (string.IsNullOrEmpty(actionStartText))
            {
                Messaging.Write(ErrorMessages.IllegalEmptyAttributeValue(sourceLineNumbers, parentElement.Name.LocalName, attribute.Name.LocalName));
                return;
            }

            XAttribute idAttrib = parentElement.Attribute("Id");
            if (idAttrib == null)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, parentElement.Name.LocalName, "Id"));
                return;
            }
            string caId = ParseHelper.GetAttributeIdentifierValue(sourceLineNumbers, idAttrib);
            if (string.IsNullOrEmpty(caId))
            {
                Messaging.Write(ErrorMessages.IllegalEmptyAttributeValue(sourceLineNumbers, parentElement.Name.LocalName, "Id"));
                return;
            }

            ActionTextSymbol row = section.AddSymbol(new ActionTextSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, caId)));
            row.Action = caId;
            row.Description = actionStartText;
            row.Template = "";
        }

        private void ParseCustomActionDataAttribute(IntermediateSection section, XElement parentElement, XAttribute attribute)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(parentElement);
            string cad = ParseHelper.GetAttributeValue(sourceLineNumbers, attribute);
            if (string.IsNullOrEmpty(cad))
            {
                Messaging.Write(ErrorMessages.IllegalEmptyAttributeValue(sourceLineNumbers, parentElement.Name.LocalName, attribute.Name.LocalName));
            }

            XAttribute executeAttrib = parentElement.Attribute("Execute");
            if (executeAttrib == null)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, parentElement.Name.LocalName, "Execute"));
                return;
            }
            string execute = ParseHelper.GetAttributeValue(sourceLineNumbers, executeAttrib);
            if (string.IsNullOrEmpty(execute) || !(execute.Equals("commit") || execute.Equals("deferred") || execute.Equals("rollback")))
            {
                // CustomActionData is only relevant for deferred actions
                Messaging.Write(ErrorMessages.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, parentElement.Name.LocalName, "Execute", execute, attribute.Name.LocalName));
                return;
            }

            XAttribute dllEntryAttrib = parentElement.Attribute("DllEntry");
            if (dllEntryAttrib == null)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, parentElement.Name.LocalName, "DllEntry"));
            }
            string dllEntry = ParseHelper.GetAttributeValue(sourceLineNumbers, dllEntryAttrib);
            if (string.IsNullOrWhiteSpace(dllEntry))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, parentElement.Name.LocalName, "DllEntry"));
                return;
            }

            XAttribute idAttrib = parentElement.Attribute("Id");
            if (idAttrib == null)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, parentElement.Name.LocalName, "Id"));
            }
            string caId = ParseHelper.GetAttributeIdentifierValue(sourceLineNumbers, idAttrib);
            if (string.IsNullOrEmpty(caId))
            {
                Messaging.Write(ErrorMessages.IllegalEmptyAttributeValue(sourceLineNumbers, parentElement.Name.LocalName, "Id"));
            }

            if (Messaging.EncounteredError)
            {
                return;
            }

            CustomActionSymbol row = section.AddSymbol(new CustomActionSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, $"Set{caId}")));
            row.ExecutionType = CustomActionExecutionType.Immediate;
            row.Source = caId;
            row.SourceType = CustomActionSourceType.Property;
            row.Target = cad;
            row.TargetType = CustomActionTargetType.TextData;

            WixActionSymbol sequenceRow = section.AddSymbol(new WixActionSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, $"InstallExecuteSequence/Set{caId}")));
            sequenceRow.Action = $"Set{caId}";
            sequenceRow.SequenceTable = SequenceTable.InstallExecuteSequence;
            sequenceRow.Condition = null; 
            sequenceRow.Before = caId;
            sequenceRow.After = null;
            sequenceRow.Overridable = false;

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", caId);

            // Set action text to the SetProperty element as well if it is set for the CA
            XAttribute caTextAttrib = parentElement.Attribute(XName.Get("ActionStartText", attribute.Name.NamespaceName));
            if (caTextAttrib != null)
            {
                string actionText = ParseHelper.GetAttributeValue(sourceLineNumbers, caTextAttrib);
                if (!string.IsNullOrWhiteSpace(actionText))
                {
                    string id = $"Set{caId}";
                    ActionTextSymbol actionText1 = section.AddSymbol(new ActionTextSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, id)));
                    actionText1.Action = id;
                    actionText1.Description = actionText;
                    actionText1.Template = "";

                    ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", id);
                }
            }
        }

        private void ParseWebsiteConfigElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string id = "web" + Guid.NewGuid().ToString("N");
            string website = null;
            bool stop = false;
            bool start = false;
            YesNoDefaultType autoStart = YesNoDefaultType.Default;
            ErrorHandling promptOnError = ErrorHandling.fail;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Website":
                            website = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Stop":
                            stop = (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                            break;

                        case "Start":
                            start = (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                            break;

                        case "AutoStart":
                            autoStart = ParseHelper.GetAttributeYesNoDefaultValue(sourceLineNumbers, attrib);
                            break;

                        case "ErrorHandling":
                            {
                                string a = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                                if (!Enum.TryParse(a, out promptOnError))
                                {
                                    Messaging.Write(ErrorMessages.IllegalAttributeValue(sourceLineNumbers, element.Name.LocalName, attrib.Name.LocalName, a));
                                    return;
                                }
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(website))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Website"));
            }

            if (Messaging.EncounteredError)
            {
                return;
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_WebsiteConfigSched");
            PSW_WebsiteConfig row = section.AddSymbol(new PSW_WebsiteConfig(sourceLineNumbers, id));
            row.Component_ = component;
            row.Website = website;
            row.Stop = stop ? 1 : 0;
            row.Start = start ? 1 : 0;
            row.AutoStart = (autoStart == YesNoDefaultType.Yes) ? 1 : (autoStart == YesNoDefaultType.No) ? 0 : -1;
            row.ErrorHandling = (int)promptOnError;
        }

        private void ParseMd5Hash(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string property = null;
            string plain = null;

            if (node.ParentNode.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(node.Parent, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Plain":
                            plain = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.ParentNode.Name.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(plain))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Plain"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    ParseHelper.UnexpectedElement(node, child);
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "Md5Hash");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_Md5Hash");
                row[0] = "md5" + Guid.NewGuid().ToString("N");
                row[1] = property;
                row[2] = plain;
            }
        }

        private void ParseToLowerCase(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string property = null;

            if (node.ParentNode.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.ParentNode.Name.LocalName, "Id"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    ParseHelper.UnexpectedElement(node, child);
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_ToLowerCase");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_ToLowerCase");
                row[0] = property;
            }
        }

        private void ParseJsonJpathSearchElement(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = "_" + Guid.NewGuid().ToString("N");
            string property = null;
            string expression = null;
            string file = null;

            if (node.ParentNode.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "JPath":
                            expression = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "FilePath":
                            file = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.ParentNode.Name.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(expression))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "JPath"));
            }
            if (string.IsNullOrEmpty(file))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "FilePath"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    ParseHelper.UnexpectedElement(node, child);
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "JsonJpathSearch");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_JsonJpathSearch");
                row[0] = id;
                row[1] = property;
                row[2] = expression;
                row[3] = file;
            }
        }

        private enum JsonFormatting
        {
            Raw,
            String,
            Boolean
        }

        private void ParseJsonJPathElement(IntermediateSection section, XElement node, string component_, string file_)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = "jpt" + Guid.NewGuid().ToString("N");
            string jpath = null;
            string value = null;
            string filePath = null;
            JsonFormatting jsonFormatting = JsonFormatting.Raw;
            ErrorHandling promptOnError = ErrorHandling.fail;

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "JPath":
                            jpath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "FilePath":
                            filePath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Value":
                            value = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Formatting":
                            string formatting = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            if (!Enum.TryParse(formatting, true, out jsonFormatting))
                            {
                                Messaging.Write(ErrorMessages.IllegalAttributeValueWithLegalList(sourceLineNumbers, node.Name.LocalName, attrib.Name.LocalName, formatting, $"{JsonFormatting.Raw}, {JsonFormatting.String}"));
                            }
                            break;

                        case "ErrorHandling":
                            {
                                string a = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                                try
                                {
                                    promptOnError = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                                }
                                catch
                                {
                                    ParseHelper.UnexpectedAttribute(attrib);
                                }
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file_) == string.IsNullOrEmpty(filePath))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeWhenNested(sourceLineNumbers, node.Name.LocalName, "FilePath", "File"));
            }
            if (string.IsNullOrEmpty(jpath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "JPath"));
            }
            if (string.IsNullOrEmpty(value))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Value"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "JsonJpathSched");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_JsonJPath");
                row[0] = id;
                row[1] = component_;
                row[2] = filePath;
                row[3] = file_;
                row[4] = jpath;
                row[5] = value;
                row[6] = (int)jsonFormatting;
                row[7] = (int)promptOnError;
            }
        }

        private void ParseDiskSpaceElement(IntermediateSection section, XElement element, string directory)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "DiskSpace");
            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_DiskSpace");
                row[0] = directory;
            }
        }

        private void ParseSetPropertyFromPipe(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string id = "_" + Guid.NewGuid().ToString("N"); // Don't care about id.
            string pipe = null;
            int timeout = 0;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "PipeName":
                            pipe = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Timeout":
                            timeout = ParseHelper.GetAttributeIntegerValue(attrib, 0, int.MaxValue);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "SetPropertyFromPipe");
            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_SetPropertyFromPipe");
                row[0] = id;
                row[1] = pipe;
                row[2] = timeout;
            }
        }

        private void ParseCreateSelfSignCertificateElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string id = null;
            string password = null;
            string x500 = null;
            string subjectAltName = null;
            int expiry = 0;
            bool deleteOnCommit = true;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName.ToLower())
                    {
                        case "id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "password":
                            password = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "expiry":
                            expiry = ParseHelper.GetAttributeIntegerValue(attrib, 0, int.MaxValue);
                            break;
                        case "x500":
                            x500 = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "deleteoncommit":
                            deleteOnCommit = (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                            break;
                        case "subjectaltname":
                            subjectAltName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute("Component", "Id"));
            }
            if (string.IsNullOrEmpty(id))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(element.Name.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(x500))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(element.Name.LocalName, "X500"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "CreateSelfSignCertificate");
            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_SelfSignCertificate");
                row[0] = id;
                row[1] = component;
                row[2] = x500;
                row[3] = subjectAltName;
                row[4] = expiry;
                row[5] = password;
                row[6] = deleteOnCommit ? 1 : 0;
            }
        }

        private enum BackupAndRestore_deferred_Schedule
        {
            BackupAndRestore_deferred_Before_InstallFiles,
            BackupAndRestore_deferred_After_DuplicateFiles,
            BackupAndRestore_deferred_After_RemoveExistingProducts
        }

        private void ParseBackupAndRestoreElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string id = null;
            string filepath = null;
            BackupAndRestore_deferred_Schedule restoreSchedule = BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_Before_InstallFiles;
            DeletePathFlags flags = 0;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName.ToLower())
                    {
                        case "id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "path":
                            filepath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ignoremissing":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= DeletePathFlags.IgnoreMissing;
                            }
                            break;
                        case "ignoreerrors":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= DeletePathFlags.IgnoreErrors;
                            }
                            break;
                        case "restorescheduling":
                            {
                                string val = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                                switch (val)
                                {
                                    case "beforeInstallFiles":
                                        restoreSchedule = BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_Before_InstallFiles;
                                        break;
                                    case "afterDuplicateFiles":
                                        restoreSchedule = BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_After_DuplicateFiles;
                                        break;
                                    case "afterRemoveExistingProducts":
                                        restoreSchedule = BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_After_RemoveExistingProducts;
                                        break;
                                    default:
                                        Messaging.Write(ErrorMessages.ValueNotSupported(sourceLineNumbers, element.Name.LocalName, attrib.Name.LocalName, val));
                                        break;
                                }
                            }
                            break;


                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "bnr" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(filepath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(element.Name.LocalName, "Path"));
            }
            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute("Component", "Id"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "BackupAndRestore");
            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "Property", restoreSchedule.ToString());

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_BackupAndRestore");
                row[0] = id;
                row[1] = component;
                row[2] = filepath;
                row[3] = (int)flags;
            }
        }

        enum InstallUtil_Bitness
        {
            asComponent = 0,
            x86 = 1,
            x64 = 2
        }

        private void ParseInstallUtilElement(IntermediateSection section, XElement node, string file)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            InstallUtil_Bitness bitness = InstallUtil_Bitness.asComponent;

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName.ToLower())
                    {
                        case "bitness":
                            string b = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            try
                            {
                                bitness = (InstallUtil_Bitness)Enum.Parse(typeof(InstallUtil_Bitness), b);
                            }
                            catch
                            {
                                ParseHelper.UnexpectedAttribute(attrib);
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute("File", "Id"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_InstallUtilSched");

            if (!Messaging.EncounteredError)
            {
                // Ensure sub-table exists for queries to succeed even if no sub-entries exist.
                Core.EnsureTable("PSW_InstallUtil_Arg");
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_InstallUtil");
                row[0] = file;
                row[1] = (int)bitness;
            }

            // Iterate child 'Argument' elements
            foreach (XElement childNode in node.ChildNodes)
            {
                if (childNode.NodeType != XmlNodeType.Element)
                {
                    continue;
                }

                XElement child = childNode as XElement;
                if (!child.Name.LocalName.Equals("Argument", StringComparison.OrdinalIgnoreCase))
                {
                    Core.UnsupportedExtensionElement(node, child);
                }

                string argId = null;
                string value = null;
                foreach (XAttribute attrib in child.Attributes())
                {
                    switch (attrib.Name.LocalName.ToLower())
                    {
                        case "id":
                            argId = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "value":
                            value = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }

                if (string.IsNullOrEmpty(argId))
                {
                    Messaging.Write(ErrorMessages.ExpectedAttribute(child.Name.LocalName, "Id"));
                    continue;
                }
                if (string.IsNullOrEmpty(value))
                {
                    Messaging.Write(ErrorMessages.ExpectedAttribute(child.Name.LocalName, "Value"));
                    continue;
                }

                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_InstallUtil_Arg");
                row[0] = file;
                row[1] = argId;
                row[2] = value;
            }
        }

        private void ParseForceVersionElement(IntermediateSection section, XElement node, string file)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string version = "65535.65535.65535.65535";

            if (node.Name.LocalName.Equals("AlwaysOverwriteFile"))
            {
                Core.OnMessage(WixWarnings.DeprecatedElement(node.Name.LocalName, "ForceVersion"));
            }

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Version":
                            version = ParseHelper.GetAttributeVersionValue(attrib, true);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute("File", "Id"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ForceVersion");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_ForceVersion");
                row[0] = file;
                row[1] = version;
            }
        }

        enum TopShelf_Account
        {
            custom = 0,
            localSystem = 1,
            localService = 2,
            networkService = 3,
            none = 4,
        }

        enum TopShelf_Start
        {
            disabled = 0,
            auto = 1,
            manual = 2,
            delayedAuto = 3,
            none = 4,
        }

        enum ErrorHandling
        {
            fail = 0,
            ignore = 1,
            prompt = 2
        }

        private void ParseTopShelfElement(IntermediateSection section, XElement node, string file)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            TopShelf_Account account = TopShelf_Account.none;
            TopShelf_Start start = TopShelf_Start.none;
            string serviceName = null;
            string displayName = null;
            string description = null;
            string instance = null;
            string userName = null;
            string password = null;
            ErrorHandling promptOnError = ErrorHandling.fail;

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Account":
                            {
                                string a = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                                try
                                {
                                    account = (TopShelf_Account)Enum.Parse(typeof(TopShelf_Account), a);
                                }
                                catch
                                {
                                    ParseHelper.UnexpectedAttribute(attrib);
                                }
                            }
                            break;

                        case "Start":
                            {
                                string a = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                                try
                                {
                                    start = (TopShelf_Start)Enum.Parse(typeof(TopShelf_Start), a);
                                }
                                catch
                                {
                                    ParseHelper.UnexpectedAttribute(attrib);
                                }
                            }
                            break;

                        case "ErrorHandling":
                            {
                                string a = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                                try
                                {
                                    promptOnError = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                                }
                                catch
                                {
                                    ParseHelper.UnexpectedAttribute(attrib);
                                }
                            }
                            break;

                        case "ServiceName":
                            serviceName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "DisplayName":
                            displayName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Description":
                            description = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Instance":
                            instance = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "UserName":
                            userName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Password":
                            password = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute("File", "Id"));
            }
            if (string.IsNullOrEmpty(userName) != (account != TopShelf_Account.custom))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, "TopShelf", "Account", "custom", "UserName"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "TopShelf");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_TopShelf");
                row[0] = file;
                row[1] = serviceName;
                row[2] = displayName;
                row[3] = description;
                row[4] = instance;
                row[5] = (int)account;
                row[6] = userName;
                row[7] = password;
                row[8] = (int)start;
                row[9] = (int)promptOnError;
            }
        }

        // Definition must match ExeOnComponent.cpp
        [Flags]
        enum ExecOnComponentFlags
        {
            None = 0,

            // Action
            OnInstall = 1,
            OnRemove = 2 * OnInstall,
            OnReinstall = 2 * OnRemove,

            // Action rollback
            OnInstallRollback = 2 * OnReinstall,
            OnRemoveRollback = 2 * OnInstallRollback,
            OnReinstallRollback = 2 * OnRemoveRollback,

            AnyAction = OnInstall | OnRemove | OnReinstall | OnInstallRollback | OnReinstallRollback | OnRemoveRollback,

            // Schedule
            BeforeStopServices = 2 * OnReinstallRollback,
            AfterStopServices = 2 * BeforeStopServices,
            BeforeStartServices = 2 * AfterStopServices,
            AfterStartServices = 2 * BeforeStartServices,

            AnyTiming = BeforeStopServices | AfterStopServices | BeforeStartServices | AfterStartServices,

            // Not waiting
            ASync = 2 * AfterStartServices,

            // Impersonate
            Impersonate = 2 * ASync,
        }

        private int GetLineNumber(SourceLineNumberCollection sourceLineNumbers)
        {
            int order = 0;
            if ((sourceLineNumbers != null) && (sourceLineNumbers.Count > 0))
            {
                foreach (SourceLineNumber line in sourceLineNumbers)
                {
                    if (line.HasLineNumber)
                    {
                        order = line.LineNumber;
                    }
                }
            }
            return order;
        }

        [Flags]
        private enum SqlExecOn
        {
            None = 0,
            Install = 1,
            InstallRollback = Install * 2,
            Uninstall = InstallRollback * 2,
            UninstallRollback = Uninstall * 2,
            Reinstall = UninstallRollback * 2,
            ReinstallRollback = Reinstall * 2,
        }

        private void ParseSqlScriptElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string id = "sql" + Guid.NewGuid().ToString("N");
            string binary = null;
            string driver = null;
            string server = null;
            string connectionString = null;
            string instance = null;
            string database = null;
            string username = null;
            string password = null;
            string port = null;
            string encrypted = null;
            ErrorHandling? errorHandling = null;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);
            SqlExecOn sqlExecOn = SqlExecOn.None;
            YesNoType aye;

            foreach (XAttribute attrib in element.Attributes())
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    continue;
                }

                switch (attrib.Name.LocalName)
                {
                    case "Id":
                        id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "BinaryKey":
                        binary = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "ConnectionString":
                        connectionString = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Driver":
                        driver = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Server":
                        server = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Instance":
                        instance = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Port":
                        port = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Encrypt":
                        encrypted = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Database":
                        database = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Username":
                        username = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Password":
                        password = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Order":
                        order = ParseHelper.GetAttributeIntegerValue(attrib, 0, 1000000000);
                        break;

                    case "ErrorHandling":
                        string a = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        try
                        {
                            errorHandling = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                        }
                        catch
                        {
                            ParseHelper.UnexpectedAttribute(attrib);
                        }
                        break;

                    case "OnInstall":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            sqlExecOn |= SqlExecOn.Install;
                        }
                        break;

                    case "OnInstallRollback":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            sqlExecOn |= SqlExecOn.InstallRollback;
                        }
                        break;

                    case "OnReinstall":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            sqlExecOn |= SqlExecOn.Reinstall;
                        }
                        break;

                    case "OnReinstallRollback":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            sqlExecOn |= SqlExecOn.ReinstallRollback;
                        }
                        break;

                    case "OnUninstall":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            sqlExecOn |= SqlExecOn.Uninstall;
                        }
                        break;

                    case "OnUninstallRollback":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            sqlExecOn |= SqlExecOn.UninstallRollback;
                        }
                        break;

                    default:
                        ParseHelper.UnexpectedAttribute(attrib);
                        break;
                }
            }

            if (!string.IsNullOrEmpty(connectionString) &&
                (!string.IsNullOrEmpty(server) || !string.IsNullOrEmpty(instance) || !string.IsNullOrEmpty(database) || !string.IsNullOrEmpty(port) || !string.IsNullOrEmpty(encrypted) || !string.IsNullOrEmpty(password) || !string.IsNullOrEmpty(username)))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeWithOtherAttribute(sourceLineNumbers, element.Name.LocalName, "ConnectionString", "any other"));
            }
            if (string.IsNullOrEmpty(server) && string.IsNullOrEmpty(connectionString))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, element.Name.LocalName, "Server", "ConnectionString"));
            }
            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute("Component", "Id"));
            }
            if (string.IsNullOrEmpty(binary))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(element.Name.LocalName, "BinaryKey"));
            }
            if (sqlExecOn == SqlExecOn.None)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(element.Name.LocalName, "OnXXX"));
            }

            // ExitCode mapping
            foreach (XElement child in element.ChildNodes)
            {
                SourceLineNumberCollection repLines = Preprocessor.GetSourceLineNumbers(child);
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    switch (child.Name.LocalName)
                    {
                        case "Replace":
                            {
                                string from = null;
                                string to = "";
                                int repOrder = 1000000000 + GetLineNumber(repLines);
                                foreach (XAttribute a in child.Attributes())
                                {
                                    if (!attrib.Name.Namespace.Equals(Namespace))
                                    {
                                        continue;
                                    }

                                    switch (a.Name.LocalName)
                                    {
                                        case "Text":
                                            from = ParseHelper.GetAttributeValue(repLines, a);
                                            break;

                                        case "Replacement":
                                            to = ParseHelper.GetAttributeValue(repLines, a, true);
                                            break;

                                        case "Order":
                                            repOrder = ParseHelper.GetAttributeIntegerValue(repLines, a, 0, 1000000000);
                                            break;
                                    }
                                }

                                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, (repLines, "PSW_SqlScript_Replacements");
                                row[0] = id;
                                row[1] = from;
                                row[2] = to;
                                row[3] = repOrder;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedElement(element, child);
                            break;
                    }
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_SqlScript");

            if (!Messaging.EncounteredError)
            {
                // Ensure sub-tables exist for queries to succeed even if no sub-entries exist.
                Core.EnsureTable("PSW_SqlScript_Replacements");
                Core.EnsureTable("PSW_SqlScript");
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_SqlScript");
                row[0] = id;
                row[1] = component;
                row[2] = binary;
                row[3] = server;
                row[4] = instance;
                row[5] = port;
                row[6] = encrypted;
                row[7] = database;
                row[8] = username;
                row[9] = password;
                row[10] = (int)sqlExecOn;
                row[11] = (int)(errorHandling ?? ErrorHandling.fail);
                row[12] = order;
                row[13] = connectionString;
                row[14] = driver;
            }
        }

        private void ParseXslTransform(IntermediateSection section, XElement element, string component, string file)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string id = "xsl" + Guid.NewGuid().ToString("N");
            string filePath = null;
            string binary = null;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);
            Microsoft.Tools.WindowsInstallerXml.Serialize.InstallUninstallType on = Microsoft.Tools.WindowsInstallerXml.Serialize.InstallUninstallType.install;

            foreach (XAttribute attrib in element.Attributes())
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    continue;
                }

                switch (attrib.Name.LocalName)
                {
                    case "Id":
                        id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "BinaryKey":
                        binary = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "FilePath":
                        filePath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "On":
                        on = ParseHelper.GetAttributeInstallUninstallValue(attrib);
                        break;

                    case "Order":
                        order = ParseHelper.GetAttributeIntegerValue(attrib, 0, 1000000000);
                        break;

                    default:
                        ParseHelper.UnexpectedAttribute(attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(file) == string.IsNullOrEmpty(filePath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributeInElementOrParent(sourceLineNumbers, element.Name.LocalName, "FilePath", "File"));
            }
            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute("Component", "Id"));
            }
            if (string.IsNullOrEmpty(binary))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(element.Name.LocalName, "BinaryKey"));
            }
            if (on == Microsoft.Tools.WindowsInstallerXml.Serialize.InstallUninstallType.NotSet)
            {
                on = Microsoft.Tools.WindowsInstallerXml.Serialize.InstallUninstallType.install;
            }

            // Text replacements in XSL
            foreach (XElement child in element.ChildNodes)
            {
                SourceLineNumberCollection repLines = Preprocessor.GetSourceLineNumbers(child);
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    switch (child.Name.LocalName)
                    {
                        case "Replace":
                            {
                                string from = null;
                                string to = "";
                                int repOrder = 1000000000 + GetLineNumber(repLines);
                                foreach (XAttribute a in child.Attributes())
                                {
                                    if (!attrib.Name.Namespace.Equals(Namespace))
                                    {
                                        continue;
                                    }

                                    switch (a.Name.LocalName)
                                    {
                                        case "Text":
                                            from = ParseHelper.GetAttributeValue(repLines, a);
                                            break;

                                        case "Replacement":
                                            to = ParseHelper.GetAttributeValue(repLines, a, true);
                                            break;

                                        case "Order":
                                            repOrder = ParseHelper.GetAttributeIntegerValue(repLines, a, 0, 1000000000);
                                            break;
                                    }
                                }

                                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, (repLines, "PSW_XslTransform_Replacements");
                                row[0] = id;
                                row[1] = from;
                                row[2] = to;
                                row[3] = repOrder;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedElement(element, child);
                            break;
                    }
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_XslTransform");

            if (!Messaging.EncounteredError)
            {
                // Ensure sub-tables exist for queries to succeed even if no sub-entries exist.
                Core.EnsureTable("PSW_XslTransform_Replacements");
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_XslTransform");
                int i = 0;
                row[i++] = id;
                row[i++] = file;
                row[i++] = component;
                row[i++] = filePath;
                row[i++] = binary;
                row[i++] = order;
                row[i++] = (int)on;
            }
        }

        private void ParseExecOnComponentElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string id = null;
            string binary = null;
            string command = null;
            string workDir = null;
            ExecOnComponentFlags flags = ExecOnComponentFlags.None;
            ErrorHandling? errorHandling = null;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);
            YesNoType aye;
            string user = null;

            if (element.HasAttribute("IgnoreExitCode") && element.HasAttribute("ErrorHandling"))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeWithOtherAttribute(sourceLineNumbers, element.Name.LocalName, "IgnoreExitCode", "ErrorHandling"));
                return;
            }

            foreach (XAttribute attrib in element.Attributes())
            {
                switch (attrib.Name.LocalName)
                {
                    case "Id":
                        id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Command":
                        command = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "BinaryKey":
                        binary = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "WorkingDirectory":
                        workDir = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Order":
                        order = ParseHelper.GetAttributeIntegerValue(attrib, 0, 1000000000);
                        break;

                    case "Impersonate":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.Impersonate;
                        }
                        break;

                    case "User":
                        user = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "IgnoreExitCode":
                        Core.OnMessage(WixWarnings.DeprecatedAttribute(element.Name.LocalName, attrib.Name.LocalName));
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            errorHandling = ErrorHandling.ignore;
                        }
                        break;

                    case "ErrorHandling":
                        string a = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        try
                        {
                            errorHandling = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                        }
                        catch
                        {
                            ParseHelper.UnexpectedAttribute(attrib);
                        }

                        if (element.HasAttribute("IgnoreExitCode"))
                        {
                            Messaging.Write(ErrorMessages.IllegalAttributeWithOtherAttribute(sourceLineNumbers, element.Name.LocalName, attrib.Name.LocalName, "IgnoreExitCode"));
                        }
                        break;

                    case "Wait":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.No)
                        {
                            flags |= ExecOnComponentFlags.ASync;
                            if (errorHandling == null)
                            {
                                errorHandling = ErrorHandling.ignore; // Really isn't checked on async, but just to be on the safe side.
                            }
                        }
                        break;

                    case "OnInstall":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnInstall;
                        }
                        break;

                    case "OnInstallRollback":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnInstallRollback;
                        }
                        break;

                    case "OnReinstall":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnReinstall;
                        }
                        break;

                    case "OnReinstallRollback":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnReinstallRollback;
                        }
                        break;

                    case "OnUninstall":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnRemove;
                        }
                        break;

                    case "OnUninstallRollback":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnRemoveRollback;
                        }
                        break;

                    case "BeforeStopServices":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.BeforeStopServices;
                        }
                        break;

                    case "AfterStopServices":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.AfterStopServices;
                        }
                        break;

                    case "BeforeStartServices":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.BeforeStartServices;
                        }
                        break;

                    case "AfterStartServices":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.AfterStartServices;
                        }
                        break;

                    default:
                        ParseHelper.UnexpectedAttribute(attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute("Component", "Id"));
            }
            if (string.IsNullOrEmpty(id))
            {
                id = "exc" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(command))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(element.Name.LocalName, "Command"));
            }
            if ((flags & ExecOnComponentFlags.AnyAction) == ExecOnComponentFlags.None)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(element.Name.LocalName, "OnXXX"));
            }
            if ((flags & ExecOnComponentFlags.AnyTiming) == ExecOnComponentFlags.None)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(element.Name.LocalName, "BeforeXXX or AfterXXX"));
            }
            if (((flags & ExecOnComponentFlags.ASync) == ExecOnComponentFlags.ASync) && (errorHandling != ErrorHandling.ignore))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, element.Name.LocalName, "Wait", "no", "ErrorHandling"));
            }

            // ExitCode mapping
            foreach (XElement child in element.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    switch (child.Name.LocalName)
                    {
                        case "ExitCode":
                            {
                                ushort from = 0, to = 0;
                                foreach (XAttribute a in child.Attributes())
                                {
                                    if (a.Name.Namespace.Equals(Namespace))
                                    {
                                        switch (a.Name.LocalName)
                                        {
                                            case "Value":
                                                from = (ushort)ParseHelper.GetAttributeIntegerValue(a, 0, 0xffff);
                                                break;

                                            case "Behavior":
                                                switch (ParseHelper.GetAttributeValue(sourceLineNumbers, a))
                                                {
                                                    case "success":
                                                        to = 0;
                                                        break;
                                                    case "scheduleReboot":
                                                        to = 3010;
                                                        break;
                                                    case "error":
                                                        to = 0x4005;
                                                        break;
                                                    default:
                                                        to = (ushort)ParseHelper.GetAttributeIntegerValue(a, 0, 0xffff);
                                                        break;
                                                }
                                                break;
                                        }
                                    }
                                }
                                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_ExecOnComponent_ExitCode");
                                row[0] = id;
                                row[1] = (int)from;
                                row[2] = (int)to;
                            }
                            break;

                        case "ConsoleOutput":
                            {
                                ErrorHandling stdoutHandling = ErrorHandling.fail;
                                string regex = null;
                                string prompt = null;
                                bool onMatch = true;

                                foreach (XAttribute a in child.Attributes())
                                {
                                    if (a.Name.Namespace.Equals(Namespace))
                                    {
                                        switch (a.Name.LocalName)
                                        {
                                            case "Expression":
                                                regex = ParseHelper.GetAttributeValue(sourceLineNumbers, a);
                                                break;

                                            case "BehaviorOnMatch":
                                                onMatch = (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, a) == YesNoType.Yes);
                                                break;

                                            case "PromptText":
                                                prompt = ParseHelper.GetAttributeValue(sourceLineNumbers, a);
                                                break;

                                            case "Behavior":
                                                if (!Enum.TryParse<ErrorHandling>(ParseHelper.GetAttributeValue(sourceLineNumbers, a), out stdoutHandling))
                                                {
                                                    ParseHelper.UnexpectedAttribute(child, a);
                                                }
                                                break;
                                        }
                                    }
                                }
                                if ((stdoutHandling == ErrorHandling.prompt) == string.IsNullOrEmpty(prompt))
                                {
                                    Messaging.Write(ErrorMessages.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, child.Name.LocalName, "Behavior", "prompt", "PromptText"));
                                }
                                if (string.IsNullOrEmpty(regex))
                                {
                                    Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, child.Name.LocalName, "Expression"));
                                }

                                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_ExecOn_ConsoleOutput");
                                row[0] = "std" + Guid.NewGuid().ToString("N");
                                row[1] = id;
                                row[2] = regex;
                                row[3] = onMatch ? 1 : 0;
                                row[4] = (int)stdoutHandling;
                                row[5] = prompt;
                            }
                            break;


                        case "Environment":
                            {
                                string name = null, value = null;

                                foreach (XAttribute a in child.Attributes())
                                {
                                    if (a.Name.Namespace.Equals(Namespace))
                                    {
                                        switch (a.Name.LocalName)
                                        {
                                            case "Value":
                                                value = ParseHelper.GetAttributeValue(sourceLineNumbers, a);
                                                break;

                                            case "Name":
                                                name = ParseHelper.GetAttributeValue(sourceLineNumbers, a);
                                                break;

                                            default:
                                                Core.UnsupportedExtensionAttribute(a);
                                                break;
                                        }
                                    }
                                }
                                if (string.IsNullOrEmpty(name) || string.IsNullOrEmpty(value))
                                {
                                    Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, child.Name.LocalName, "Name", "Value"));
                                    break;
                                }

                                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_ExecOnComponent_Environment");
                                row[0] = id;
                                row[1] = name;
                                row[2] = value;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedElement(element, child);
                            break;
                    }
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ExecOnComponent");

            if (!Messaging.EncounteredError)
            {
                // Ensure sub-tables exist for queries to succeed even if no sub-entries exist.
                Core.EnsureTable("PSW_ExecOnComponent_ExitCode");
                Core.EnsureTable("PSW_ExecOn_ConsoleOutput");
                Core.EnsureTable("PSW_ExecOnComponent_Environment");
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_ExecOnComponent");
                row[0] = id;
                row[1] = component;
                row[2] = binary;
                row[3] = command;
                row[4] = workDir;
                row[5] = (int)flags;
                row[6] = (int)(errorHandling ?? ErrorHandling.fail);
                row[7] = order;
                row[8] = user;
            }
        }

        enum ServiceStart : int
        {
            boot = 0,
            unchanged = -1,
            auto = 2,
            demand = 3,
            disabled = 4,
            system = 1,

            autoDelayed = 5
        }

        private void ParseServiceConfigElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string id = null;
            string service = null;
            string commandLine = null;
            string account = null;
            string password = null;
            string loadOrderGroup = null;
            ServiceStart start = ServiceStart.unchanged;
            ErrorHandling errorHandling = ErrorHandling.fail;

            foreach (XAttribute attrib in element.Attributes())
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }

                switch (attrib.Name.LocalName)
                {
                    case "Id":
                        id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "ServiceName":
                        service = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "CommandLine":
                        commandLine = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Account":
                        account = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Password":
                        password = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Start":
                        start = (ServiceStart)Enum.Parse(typeof(ServiceStart), id = ParseHelper.GetAttributeValue(sourceLineNumbers, );
                        break;

                    case "LoadOrderGroup":
                        loadOrderGroup = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "ErrorHandling":
                        {
                            string a = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            try
                            {
                                errorHandling = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                            }
                            catch
                            {
                                ParseHelper.UnexpectedAttribute(attrib);
                            }
                        }
                        break;

                    default:
                        ParseHelper.UnexpectedAttribute(attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute("Component", "Id"));
            }
            if (string.IsNullOrEmpty(service))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(element.Name.LocalName, "ServiceName"));
            }
            if (string.IsNullOrEmpty(account) && !string.IsNullOrEmpty(password))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributesWithOtherAttribute(sourceLineNumbers, element.Name.LocalName, "Password", "Account"));
            }
            if (string.IsNullOrEmpty(id))
            {
                id = "svc" + Guid.NewGuid().ToString("N");
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_ServiceConfig");

            if (!Messaging.EncounteredError)
            {
                // Ensure sub-table exists for queries to succeed even if no sub-entries exist.
                Core.EnsureTable("PSW_ServiceConfig_Dependency");
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_ServiceConfig");
                row[0] = id;
                row[1] = component;
                row[2] = service;
                row[3] = commandLine;
                row[4] = account;
                row[5] = password;
                row[6] = (start == ServiceStart.autoDelayed) ? (int)ServiceStart.auto : (int)start;
                row[7] = (start == ServiceStart.autoDelayed) ? 1 : (start == ServiceStart.auto) ? 0 : -1;
                row[8] = loadOrderGroup;
                row[9] = (int)errorHandling;
            }

            foreach (XElement child in element.ChildNodes)
            {
                if (child.NamespaceURI != element.NamespaceURI)
                {
                    continue;
                }

                if (!child.Name.LocalName.Equals("Dependency"))
                {
                    Core.UnsupportedExtensionElement(element, child);
                    continue;
                }

                foreach (XAttribute attrib in child.Attributes())
                {
                    if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                    {
                        Core.UnsupportedExtensionAttribute(attrib);
                    }
                    string depService = null;
                    string group = null;

                    switch (attrib.Name.LocalName)
                    {
                        case "Service":
                            depService = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Group":
                            group = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }

                    if (string.IsNullOrEmpty(depService) && string.IsNullOrEmpty(group))
                    {
                        Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, child.Name.LocalName, "Service", "Group"));
                    }

                    if (!Messaging.EncounteredError)
                    {
                        IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_ServiceConfig_Dependency");
                        row[0] = id;
                        row[1] = depService;
                        row[2] = group;
                    }
                }
            }
        }

        private void ParseDismElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string id = null;
            string features = null;
            string exclude = null;
            string package = null;
            ErrorHandling promptOnError = ErrorHandling.fail;
            int cost = 20971520; // 20 MB.

            foreach (XAttribute attrib in element.Attributes())
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }

                switch (attrib.Name.LocalName)
                {
                    case "EnableFeature":
                        features = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "ExcludeFeatures":
                        exclude = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "PackagePath":
                        package = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Id":
                        id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Cost":
                        cost = ParseHelper.GetAttributeIntegerValue(attrib, 0, int.MaxValue);
                        break;

                    case "ErrorHandling":
                        {
                            string a = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            try
                            {
                                promptOnError = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                            }
                            catch
                            {
                                ParseHelper.UnexpectedAttribute(attrib);
                            }
                        }
                        break;

                    default:
                        ParseHelper.UnexpectedAttribute(attrib);
                        break;
                }
            }

            foreach (XElement child in element.ChildNodes)
            {
                Core.UnsupportedExtensionElement(element, child);
            }

            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute("Component", "Id"));
            }
            if (string.IsNullOrEmpty(features) && string.IsNullOrEmpty(package))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, element.Name.LocalName, "EnableFeature", "PackagePath"));
            }
            if (string.IsNullOrEmpty(id))
            {
                id = "dsm" + Guid.NewGuid().ToString("N");
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "DismSched");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_Dism");
                row[0] = id;
                row[1] = component;
                row[2] = features;
                row[3] = exclude;
                row[4] = package;
                row[5] = cost;
                row[6] = (int)promptOnError;
            }
        }

        private void ParseTaskSchedulerElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string id = "tsk" + Guid.NewGuid().ToString("N");
            string taskXml = null;
            string taskName = null;
            string user = null;
            string password = null;

            foreach (XAttribute attrib in element.Attributes())
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }

                switch (attrib.Name.LocalName)
                {
                    case "TaskName":
                        taskName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "User":
                        user = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Password":
                        password = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "XmlFile":
                        taskXml = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    default:
                        ParseHelper.UnexpectedAttribute(attrib);
                        break;
                }
            }

            foreach (XElement child in element.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(element, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(element, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    if (!string.IsNullOrWhiteSpace(taskXml))
                    {
                        Messaging.Write(ErrorMessages.IllegalAttributeWithInnerText(sourceLineNumbers, element.Name.LocalName, "XmlFile"));
                    }
                    taskXml = child.Value;
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute("Component", "Id"));
            }
            if (string.IsNullOrWhiteSpace(taskXml))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributeOrElement(sourceLineNumbers, element.Name.LocalName, "XmlFile", "Inner text or CDATA"));
            }
            if (string.IsNullOrEmpty(taskName))
            {
                Messaging.Write(ErrorMessages.ExpectedElement(sourceLineNumbers, element.Name.LocalName, "TaskName"));
            }
            if (string.IsNullOrEmpty(user) && !string.IsNullOrEmpty(password))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributesWithOtherAttribute(sourceLineNumbers, element.Name.LocalName, "User", "Password"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "TaskScheduler");

            if (!Messaging.EncounteredError)
            {
                taskXml = taskXml.Trim();
                taskXml = taskXml.Replace("\r", "");
                taskXml = taskXml.Replace("\n", "");
                taskXml = taskXml.Replace(Environment.NewLine, "");

                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_TaskScheduler");
                row[0] = id;
                row[1] = taskName;
                row[2] = component;
                row[3] = taskXml;
                row[4] = user;
                row[5] = password;
            }
        }

        [Flags]
        private enum CustomUninstallKeyAttributes
        {
            None = 0,
            Write = 1,
            Delete = 2
        }

        private void ParseCustomUninstallKeyElement(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string productCode = null;
            string name = null;
            string data = null;
            string datatype = "REG_SZ";
            string id = null;
            string condition = null;
            CustomUninstallKeyAttributes attributes = CustomUninstallKeyAttributes.None;

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName.ToLower())
                    {
                        case "id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            if (string.IsNullOrEmpty(name))
                            {
                                name = id;
                            }
                            break;
                        case "productcode":
                            productCode = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "name":
                            name = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "data":
                            data = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "datatype":
                            datatype = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "operation":
                            if (id = ParseHelper.GetAttributeValue(sourceLineNumbers, .Equals("delete", StringComparison.OrdinalIgnoreCase))
                            {
                                attributes |= CustomUninstallKeyAttributes.Delete;
                            }
                            if (id = ParseHelper.GetAttributeValue(sourceLineNumbers, .Equals("write", StringComparison.OrdinalIgnoreCase))
                            {
                                attributes |= CustomUninstallKeyAttributes.Write;
                            }
                            break;
                        case "condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "uni" + Guid.NewGuid().ToString("N");
            }

            if (string.IsNullOrEmpty(name))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Name"));
            }

            if (string.IsNullOrEmpty(data))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Data"));
            }

            if (string.IsNullOrEmpty(datatype))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "DataType"));
            }

            if (attributes == CustomUninstallKeyAttributes.None)
            {
                attributes = CustomUninstallKeyAttributes.Write;
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement("text", $"Condition attribute in {node.Name.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "CustomUninstallKey_Immediate");
            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "CustomUninstallKey_deferred");
            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "CustomUninstallKey_rollback");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_CustomUninstallKey");
                row[0] = id;
                row[1] = productCode;
                row[2] = name;
                row[3] = data;
                row[4] = datatype;
                row[5] = (int)attributes;
                row[6] = condition;
            }
        }

        private enum ReadIniValuesAttributes
        {
            None = 0,
            IgnoreErrors = 1
        }

        private void ParseReadIniValuesElement(IntermediateSection section, XElement node, XElement parent)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = null;
            string DestProperty = null;
            string FilePath = null;
            string Section = null;
            string Key = null;
            YesNoType IgnoreErrors = YesNoType.No;
            string condition = null;

            if ((parent != null) && parent.Name.LocalName.Equals("Property"))
            {
                DestProperty = ParseHelper.GetAttributeIdentifier(sourceLineNumbers, parent.Attribute("Id")).Id;
            }

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "DestProperty":
                            if (!string.IsNullOrEmpty(DestProperty))
                            {
                                Messaging.Write(ErrorMessages.ExpectedAttributeInElementOrParent(sourceLineNumbers, node.Name.LocalName, attrib.Name.LocalName, parent.Name.LocalName));
                                return;
                            }
                            DestProperty = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "FilePath":
                            FilePath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Section":
                            Section = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Key":
                            Key = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "IgnoreErrors":
                            IgnoreErrors = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(node, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "ini" + Guid.NewGuid().ToString("N");
            }

            if (string.IsNullOrEmpty(DestProperty))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "DestProperty"));
            }
            if (!DestProperty.ToUpper().Equals(DestProperty))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", DestProperty));
            }

            if (string.IsNullOrEmpty(FilePath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "FilePath"));
            }

            if (string.IsNullOrEmpty(Key))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Key"));
            }

            if (string.IsNullOrEmpty(Section))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Section"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement("text", $"Condition attribute in {node.Name.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ReadIniValues");

            if (!Messaging.EncounteredError)
            {
                PSW_ReadIniValues row = section.AddSymbol(section, sourceLineNumbers, id);
                row.FilePath = FilePath;
                row[2] = Section;
                row[3] = Key;
                row[4] = DestProperty;
                row[5] = (IgnoreErrors == YesNoType.Yes) ? 1 : 0;
                row[6] = condition;
            }
        }

        private enum RegistryArea
        {
            x86,
            x64,
            Default
        }

        private void ParseRemoveRegistryValue(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = null;
            string root = null;
            string key = null;
            string name = null;
            RegistryArea area = RegistryArea.Default;
            string condition = "";

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName.ToLower())
                    {
                        case "id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "root":
                            root = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "key":
                            key = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "name":
                            name = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "area":
                            try
                            {
                                area = (RegistryArea)Enum.Parse(typeof(RegistryArea), id = ParseHelper.GetAttributeValue(sourceLineNumbers, );
                            }
                            catch
                            {
                                Messaging.Write(ErrorMessages.ValueNotSupported(sourceLineNumbers, node.Name.LocalName, "Area", id = ParseHelper.GetAttributeValue(sourceLineNumbers, ));
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "reg" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(key))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Key"));
            }
            if (string.IsNullOrEmpty(root))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Root"));
            }
            if (string.IsNullOrEmpty(name))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Name"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "RemoveRegistryValue_Immediate");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_RemoveRegistryValue");
                row[0] = id;
                row[1] = root;
                row[2] = key;
                row[3] = name;
                row[4] = area.ToString();
                row[5] = 0;
                row[6] = condition;
            }
        }

        private void ParseCertificateHashSearchElement(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string property = null;
            string certName = null;
            string friendlyName = null;
            string issuer = null;
            string serial = null;

            if (node.ParentNode.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "CertName":
                            certName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "FriendlyName":
                            friendlyName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Issuer":
                            issuer = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "SerialNumber":
                            serial = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.ParentNode.Name.LocalName, "Id"));
            }

            // At least CertName OR (Issuer AND Serial) OR FriendlyName
            if (string.IsNullOrEmpty(issuer) != string.IsNullOrEmpty(serial))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, node.Name.LocalName, "Issuer", "SerialNumber"));
            }
            if (string.IsNullOrEmpty(certName) && string.IsNullOrEmpty(issuer) && string.IsNullOrEmpty(friendlyName))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, node.Name.LocalName, "Issuer", "SerialNumber", "CertName", "FriendlyName"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    ParseHelper.UnexpectedElement(node, child);
                }
            }

            if (!Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "CertificateHashSearch");

                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_CertificateHashSearch");
                row[0] = property;
                row[1] = certName;
                row[2] = friendlyName;
                row[3] = issuer;
                row[4] = serial;
            }
        }

        private void ParseEvaluateElement(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = "_" + Guid.NewGuid().ToString("N");
            string property = null;
            string expression = null;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            if (node.ParentNode.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Expression":
                            expression = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(attrib, 0, 1000000000);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.ParentNode.Name.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(expression))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Expression"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    ParseHelper.UnexpectedElement(node, child);
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "EvaluateExpression");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_EvaluateExpression");
                row[0] = id;
                row[1] = property;
                row[2] = expression;
                row[3] = order;
            }
        }

        private void ParsePathSearchElement(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = "pth" + Guid.NewGuid().ToString("N"); ;
            string file = null;
            string property = null;

            if (node.ParentNode.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(node.ParentNode, node);
                return;
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "FileName":
                            file = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.ParentNode.Name.LocalName, "Id"));
                return;
            }
            if (string.IsNullOrEmpty(file))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "FileName"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_PathSearch");
            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_PathSearch");
                row[0] = id;
                row[1] = property;
                row[2] = file;
            }
        }

        private void ParseVersionCompareElement(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = "ver" + Guid.NewGuid().ToString("N"); ;
            string version1 = null;
            string version2 = null;
            string property = null;

            if (node.ParentNode.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(node.ParentNode, node);
                return;
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Version1":
                            version1 = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Version2":
                            version2 = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.ParentNode.Name.LocalName, "Id"));
                return;
            }
            if (string.IsNullOrEmpty(version1))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Version1"));
            }
            if (string.IsNullOrEmpty(version2))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Version2"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_VersionCompare");
            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_VersionCompare");
                row[0] = id;
                row[1] = property;
                row[2] = version1;
                row[3] = version2;
            }
        }

        private void ParseAccountSidSearchElement(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = "sid" + Guid.NewGuid().ToString("N"); ;
            string systemName = null;
            string accountName = null;
            string property = null;
            string condition = "";

            if (node.ParentNode.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(node.ParentNode, node);
                return;
            }
            property = node.ParentNode.Attributes["Id"].Value;

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "AccountName":
                            accountName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "SystemName":
                            systemName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.ParentNode.Name.LocalName, "Id"));
                return;
            }
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }
            if (string.IsNullOrEmpty(accountName))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "AccountName"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement("text", $"Condition attribute in {node.Name.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "AccountSidSearch");
            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_AccountSidSearch");
                row[0] = id;
                row[1] = property;
                row[2] = systemName;
                row[3] = accountName;
                row[4] = condition;
            }
        }

        private enum XmlSearchMatch
        {
            first,
            all,
            enforceSingle
        }

        private void ParseXmlSearchElement(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = null;
            string filePath = null;
            string xpath = null;
            string property;
            string lang = null;
            string namespaces = null;
            XmlSearchMatch match = XmlSearchMatch.first;
            string condition = "";

            if (node.ParentNode.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName.ToLower())
                    {
                        case "id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "filepath":
                            filePath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "xpath":
                            xpath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "language":
                            lang = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "namespaces":
                            namespaces = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "match":
                            try
                            {
                                match = (XmlSearchMatch)Enum.Parse(typeof(XmlSearchMatch), id = ParseHelper.GetAttributeValue(sourceLineNumbers, );
                            }
                            catch
                            {
                                Messaging.Write(ErrorMessages.ValueNotSupported(sourceLineNumbers, node.Name.LocalName, "Match", id = ParseHelper.GetAttributeValue(sourceLineNumbers, ));
                            }
                            break;
                        case "condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.ParentNode.Name.LocalName, "Id"));
            }
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }
            if (string.IsNullOrEmpty(id))
            {
                id = "xms" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(filePath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "FilePath"));
            }
            if (string.IsNullOrEmpty(xpath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "XPath"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement("text", $"Condition attribute in {node.Name.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "XmlSearch");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_XmlSearch");
                row[0] = id;
                row[1] = property;
                row[2] = filePath;
                row[3] = xpath;
                row[4] = lang;
                row[5] = namespaces;
                row[6] = (int)match;
                row[7] = condition;
            }
        }

        private void ParseSqlSearchElement(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = "sql" + Guid.NewGuid().ToString("N");
            string property = null;
            string server = null;
            string connectionString = null;
            string instance = null;
            string database = null;
            string username = null;
            string password = null;
            string query = null;
            string condition = null;
            string port = null;
            string encrypted = null;
            ErrorHandling errorHandling = ErrorHandling.fail;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            if (!node.ParentNode.Name.LocalName.Equals("Property"))
            {
                ParseHelper.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (string.IsNullOrWhiteSpace(property))
            {
                Messaging.Write(ErrorMessages.ParentElementAttributeRequired(sourceLineNumbers, node.ParentNode.Name.LocalName, "Id", node.Name.LocalName));
            }
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "ConnectionString":
                            connectionString = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Server":
                            server = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Instance":
                            instance = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Database":
                            database = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Username":
                            username = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Password":
                            password = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Query":
                            query = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(attrib, 0, 1000000000);
                            break;
                        case "Port":
                            port = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Encrypt":
                            encrypted = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ErrorHandling":
                            {
                                string a = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                                try
                                {
                                    errorHandling = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                                }
                                catch
                                {
                                    ParseHelper.UnexpectedAttribute(attrib);
                                }
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (!string.IsNullOrEmpty(connectionString) &&
                (!string.IsNullOrEmpty(server) || !string.IsNullOrEmpty(instance) || !string.IsNullOrEmpty(database) || !string.IsNullOrEmpty(port) || !string.IsNullOrEmpty(encrypted) || !string.IsNullOrEmpty(password) || !string.IsNullOrEmpty(username)))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeWithOtherAttribute(sourceLineNumbers, node.Name.LocalName, "ConnectionString", "any other"));
            }

            if (string.IsNullOrEmpty(server) && string.IsNullOrEmpty(connectionString))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, node.Name.LocalName, "Server", "ConnectionString"));
            }
            if (string.IsNullOrEmpty(query))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Query"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "SqlSearch");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_SqlSearch");
                int i = 0;
                row[i++] = id;
                row[i++] = property;
                row[i++] = server;
                row[i++] = instance;
                row[i++] = port;
                row[i++] = encrypted;
                row[i++] = database;
                row[i++] = username;
                row[i++] = password;
                row[i++] = query;
                row[i++] = condition;
                row[i++] = order;
                row[i++] = (int)errorHandling;
                row[i++] = connectionString;
            }
        }

        private void ParseWmiSearchElement(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = "wmi" + Guid.NewGuid().ToString("N");
            string property = null;
            string nmspace = null;
            string query = null;
            string resultProp = null;
            string condition = null;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            if (!node.ParentNode.Name.LocalName.Equals("Property"))
            {
                ParseHelper.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (string.IsNullOrWhiteSpace(property))
            {
                Messaging.Write(ErrorMessages.ParentElementAttributeRequired(sourceLineNumbers, node.ParentNode.Name.LocalName, "Id", node.Name.LocalName));
            }
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Namespace":
                            nmspace = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Query":
                            query = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ResultProperty":
                            resultProp = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(attrib, 0, 1000000000);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(query))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Query"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "WmiSearch");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_WmiSearch");
                int i = 0;
                row[i++] = id;
                row[i++] = property;
                row[i++] = condition;
                row[i++] = nmspace;
                row[i++] = query;
                row[i++] = resultProp;
                row[i++] = order;
            }
        }

        [Flags]
        private enum ExecutePhase
        {
            None = 0,
            OnExecute = 1,
            OnCommit = 2,
            OnRollback = 4,
            Secure = 8
        }

        private void ParseTelemetry(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = null;
            string url = null;
            string page = null;
            string method = null;
            string data = null;
            ExecutePhase flags = ExecutePhase.None;
            string condition = "";

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName.ToLower())
                    {
                        case "id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "url":
                            url = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "page":
                            page = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "method":
                            method = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "data":
                            data = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "onsuccess":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnCommit;
                            }
                            break;
                        case "onstart":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnExecute;
                            }
                            break;
                        case "onfailure":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnRollback;
                            }
                            break;
                        case "secure":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.Secure;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "tlm" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(url))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Url"));
            }
            if (string.IsNullOrEmpty(method))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Method"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "Telemetry");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_Telemetry");
                row[0] = id;
                row[1] = url;
                row[2] = page ?? "";
                row[3] = method;
                row[4] = data ?? "";
                row[5] = (int)flags;
                row[6] = condition;
            }
        }

        private void ParseShellExecute(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = null;
            string target = null;
            string args = "";
            string workDir = "";
            string verb = "";
            int wait = 0;
            int show = 0;
            ExecutePhase flags = ExecutePhase.None;
            string condition = "";

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName.ToLower())
                    {
                        case "id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "target":
                            target = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "args":
                            args = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "workingdir":
                            workDir = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "verb":
                            verb = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "wait":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                wait = 1;
                            }
                            break;
                        case "show":
                            show = ParseHelper.GetAttributeIntegerValue(attrib, 0, 15);
                            break;

                        case "oncommit":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnCommit;
                            }
                            break;
                        case "onexecute":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnExecute;
                            }
                            break;
                        case "onrollback":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnRollback;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "shl" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(target))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Target"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // Default to execute deferred.
            if (flags == ExecutePhase.None)
            {
                flags = ExecutePhase.OnExecute;
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ShellExecute_Immediate");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_ShellExecute");
                row[0] = id;
                row[1] = target;
                row[2] = args;
                row[3] = verb;
                row[4] = workDir;
                row[5] = show;
                row[6] = wait;
                row[7] = (int)flags;
                row[8] = condition;
            }
        }

        private void ParseMsiSqlQuery(IntermediateSection section, XElement node, XElement parent)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = null;
            string query = null;
            string condition = null;
            string property = null;

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Query":
                            query = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if ((parent != null) && parent.Name.LocalName.Equals("Property"))
            {
                property = parent.Attributes["Id"]?.Value;
                if (!property.ToUpper().Equals(property))
                {
                    Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "msq" + Guid.NewGuid().ToString("N");
            }

            if (string.IsNullOrEmpty(query))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Query"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement("text", $"Condition attribute in {node.Name.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "MsiSqlQuery");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_MsiSqlQuery");
                row[0] = id;
                row[1] = property;
                row[2] = query;
                row[3] = condition;
            }
        }

        [Flags]
        enum RegexSearchFlags
        {
            Search = 0
            , Replace = 1
        };

        [Flags]
        enum RegexResultFlags
        {
            MustMatch = 1
        };

        [Flags]
        enum RegexMatchFlags
        {
            IgnoreCare = 1
            , Extended = 2
        };

        private void ParseRegularExpression(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = null;
            string filepath = null;
            string input = null;
            string regex = null;
            string replacement = null;
            string prop = null;
            int flags = 0;
            string condition = null;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            if (node.ParentNode.Name.LocalName == "Property")
            {
                prop = node.ParentNode.Attributes["Id"].Value;
            }

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "FilePath":
                            filepath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Input":
                            input = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Expression":
                            regex = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Replacement":
                            replacement = ParseHelper.GetAttributeValue(attrib, true);
                            flags |= (int)RegexSearchFlags.Replace;
                            break;
                        case "DstProperty":
                            if (!string.IsNullOrEmpty(prop))
                            {
                                Messaging.Write(ErrorMessages.ExpectedAttributeInElementOrParent(sourceLineNumbers, node.Name.LocalName, attrib.Name.LocalName, node.ParentNode.Name.LocalName));
                            }
                            prop = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "IgnoreCase":
                            flags |= (int)RegexMatchFlags.IgnoreCare << 2;
                            break;
                        case "Extended":
                            flags |= (int)RegexMatchFlags.Extended << 2;
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(attrib, 0, 1000000000);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "rgx" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(regex))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Expression"));
            }
            if (string.IsNullOrEmpty(prop))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "DstProperty"));
            }
            if (!prop.ToUpper().Equals(prop))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", prop));
            }
            if (string.IsNullOrEmpty(input) == string.IsNullOrEmpty(filepath))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeWithOtherAttribute(sourceLineNumbers, node.Name.LocalName, "Input", "FilePath"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                // Condition can be specified on attribute 'Condition' in which case embedded text may be the property default value.
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement("text", $"Condition attribute in {node.Name.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "RegularExpression");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_RegularExpression");
                row[0] = id;
                row[1] = filepath;
                row[2] = input;
                row[3] = regex;
                row[4] = replacement;
                row[5] = prop;
                row[6] = flags;
                row[7] = condition;
                row[8] = order;
            }
        }

        private void ParseRestartLocalResources(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = "rst" + Guid.NewGuid().ToString("N");
            string path = null;
            string condition = null;

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Path":
                            path = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(path))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Path"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                }
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    condition = child.Value.Trim();
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "RestartLocalResources");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_RestartLocalResources");
                row[0] = id;
                row[1] = path;
                row[2] = condition;
            }
        }

        private enum FileEncoding
        {
            AutoDetect,
            MultiByte,
            Unicode,
            ReverseUnicode
        };

        private void ParseFileRegex(IntermediateSection section, XElement node, string component, string fileId)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = null;
            string filepath = null;
            string condition = null;
            string regex = null;
            string replacement = null;
            FileEncoding encoding = FileEncoding.AutoDetect;
            bool ignoreCase = false;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "FilePath":
                            filepath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Regex":
                            regex = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Replacement":
                            replacement = ParseHelper.GetAttributeValue(attrib, true);
                            break;
                        case "IgnoreCase":
                            ignoreCase = true;
                            break;
                        case "Encoding":
                            string enc = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            encoding = (FileEncoding)Enum.Parse(typeof(FileEncoding), enc);
                            break;
                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(attrib, 0, 1000000000);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "frx" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(regex))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Regex"));
            }
            if (string.IsNullOrEmpty(fileId) == string.IsNullOrEmpty(filepath))
            {
                // Either under File or specify FilePath, not both
                Messaging.Write(ErrorMessages.IllegalAttributeWhenNested(sourceLineNumbers, node.Name.LocalName, "FilePath", "Product"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement("text", $"Condition attribute in {node.Name.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "FileRegex_Immediate");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_FileRegex");
                row[0] = id;
                row[1] = component;
                row[2] = fileId;
                row[3] = filepath;
                row[4] = regex;
                row[5] = replacement ?? "";
                row[6] = ignoreCase ? 1 : 0;
                row[7] = (int)encoding;
                row[8] = condition;
                row[9] = order;
            }
        }

        [Flags]
        private enum DeletePathFlags
        {
            IgnoreMissing = 1,
            IgnoreErrors = 2 * IgnoreMissing,
            OnlyIfEmpty = 2 * IgnoreErrors,
            AllowReboot = 2 * OnlyIfEmpty
        }

        private void ParseDeletePath(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = null;
            string filepath = null;
            string condition = null;
            DeletePathFlags flags = DeletePathFlags.AllowReboot | DeletePathFlags.IgnoreErrors | DeletePathFlags.IgnoreMissing;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Path":
                            filepath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "IgnoreMissing":
                        case "IgnoreErrors":
                            Core.OnMessage(WixWarnings.DeprecatedAttribute(node.Name.LocalName, attrib.Name.LocalName));
                            break;
                        case "OnlyIfEmpty":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= DeletePathFlags.OnlyIfEmpty;
                            }
                            break;
                        case "AllowReboot":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.No)
                            {
                                flags &= ~DeletePathFlags.AllowReboot;
                            }
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "dlt" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(filepath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "Path"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            if (flags.HasFlag(DeletePathFlags.AllowReboot))
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_CheckRebootRequired");
            }
            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "DeletePath");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_DeletePath");
                row[0] = id;
                row[1] = filepath;
                row[2] = (int)flags;
                row[3] = condition;
                row[4] = order;
            }
        }

        private void ParseZipFile(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = null;
            string dstZipFile = null;
            string srcDir = null;
            string filePattern = "*.*";
            bool recursive = true;
            string condition = null;

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "TargetZipFile":
                            dstZipFile = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "SourceFolder":
                            srcDir = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "FilePattern":
                            filePattern = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Recursive":
                            recursive = (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "zip" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(dstZipFile))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "TargetZipFile"));
            }
            if (string.IsNullOrEmpty(srcDir))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "SourceFolder"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ZipFileSched");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_ZipFile");
                row[0] = id;
                row[1] = dstZipFile;
                row[2] = srcDir;
                row[3] = filePattern;
                row[4] = recursive ? 1 : 0;
                row[5] = condition;
            }
        }

        [Flags]
        enum UnzipFlags
        {
            // Overwrite mode
            Never = 0,
            Always = 0x1,
            Unmodified = 0x2,
            OverwriteMask = 0x3,

            // Delete ZIP after extract
            Delete = 0x10,

            // 
            CreateRoot = 0x20,

            OnRollback = 0x40,
        };

        private void ParseUnzip(IntermediateSection section, XElement node)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(node);
            string id = null;
            string zipFile = null;
            string dstDir = null;
            string condition = null;
            UnzipFlags flags = UnzipFlags.Unmodified | UnzipFlags.CreateRoot;

            foreach (XAttribute attrib in node.Attributes())
            {
                if (attrib.Name.Namespace.Equals(Namespace))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Id":
                            id = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ZipFile":
                            zipFile = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "TargetFolder":
                            dstDir = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "CreateRootFolder":
                            {
                                YesNoType aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                                if (aye == YesNoType.No)
                                {
                                    flags &= ~UnzipFlags.CreateRoot;
                                }
                            }
                            break;
                        case "DeleteZip":
                            {
                                YesNoType aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                                if (aye == YesNoType.Yes)
                                {
                                    flags |= UnzipFlags.Delete;
                                }
                            }
                            break;
                        case "OnRollback":
                            {
                                YesNoType aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                                if (aye == YesNoType.Yes)
                                {
                                    flags |= UnzipFlags.OnRollback;
                                }
                            }
                            break;
                        case "Overwrite":
                            {
                                YesNoDefaultType aye = ParseHelper.GetAttributeYesNoDefaultValue(sourceLineNumbers, attrib);
                                flags = ((aye == YesNoDefaultType.Yes) ? ((flags & ~UnzipFlags.OverwriteMask) | UnzipFlags.Always)
                                    : (aye == YesNoDefaultType.No) ? (flags & ~UnzipFlags.OverwriteMask)
                                    : ((flags & ~UnzipFlags.OverwriteMask) | UnzipFlags.Unmodified));
                            }
                            break;
                        case "OverwriteMode":
                            string b = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            try
                            {
                                UnzipFlags f = (UnzipFlags)Enum.Parse(typeof(UnzipFlags), b);
                                flags = ((flags & ~UnzipFlags.OverwriteMask) | f);
                            }
                            catch
                            {
                                ParseHelper.UnexpectedAttribute(attrib);
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "uzp" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(zipFile))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "ZipFile"));
            }
            if (string.IsNullOrEmpty(dstDir))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(node.Name.LocalName, "TargetFolder"));
            }

            // find unexpected child elements
            foreach (XElement child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        ParseHelper.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "UnzipSched");

            if (!Messaging.EncounteredError)
            {
                IntermediateSymbol row = ParseHelper.CreateSymbol(section, sourceLineNumbers, ("PSW_Unzip");
                row[0] = id;
                row[1] = zipFile;
                row[2] = dstDir;
                row[3] = (int)flags;
                row[4] = condition;
            }
        }
    }
}
