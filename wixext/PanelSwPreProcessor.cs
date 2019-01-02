using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Xml;
using Microsoft.Tools.WindowsInstallerXml;
using System.Runtime.InteropServices;

namespace PanelSw.Wix.Extensions
{
    class PanelSwPreProcessor : PreprocessorExtension
    {
        #region Split command line to arguments

        [DllImport("shell32.dll", SetLastError = true)]
        static extern IntPtr CommandLineToArgvW([MarshalAs(UnmanagedType.LPWStr)] string lpCmdLine, out int pNumArgs);

        private static string[] CommandLineToArgs(string commandLine)
        {
            int argc;
            var argv = CommandLineToArgvW(commandLine, out argc);
            if (argv == IntPtr.Zero)
            {
                throw new System.ComponentModel.Win32Exception();
            }
            try
            {
                var args = new string[argc];
                for (var i = 0; i < args.Length; i++)
                {
                    var p = Marshal.ReadIntPtr(argv, i * IntPtr.Size);
                    args[i] = Marshal.PtrToStringUni(p);
                }
                return args;
            }
            finally
            {
                Marshal.FreeHGlobal(argv);
            }
        }

        #endregion

        private string[] prefixes_ = new string[] { "tuple", "endtuple", "tuple_range", "tuple_assert", "heat" };
        public override string[] Prefixes => prefixes_;

        Dictionary<string, List<string>> tuples_ = new Dictionary<string, List<string>>();
        public override void FinalizePreprocess()
        {
            tuples_.Clear();
        }

        public override void InitializePreprocess()
        {
            tuples_.Clear();
        }

        public override bool ProcessPragma(SourceLineNumberCollection sourceLineNumbers, string prefix, string pragma, string args, XmlWriter writer)
        {
            switch (prefix)
            {
                case "tuple":
                    CreateTuple(sourceLineNumbers, pragma, args);
                    return true;

                case "endtuple":
                    DeleteTuple(sourceLineNumbers, pragma);
                    return true;

                case "tuple_assert":
                    TupleAssert(sourceLineNumbers, pragma, args);
                    return true;

                case "heat":
                    ProcessHeat(sourceLineNumbers, pragma, args, writer);
                    return true;

                default:
                    return false;
            }
        }

        private void DeleteTuple(SourceLineNumberCollection sourceLineNumbers, string pragma)
        {
            if (!tuples_.Remove(pragma))
            {
                throw new WixException(WixErrors.PreprocessorExtensionPragmaFailed(sourceLineNumbers, pragma, $"endtuple pragma for undefined tuple"));
            }
        }

        private void CreateTuple(SourceLineNumberCollection sourceLineNumbers, string pragma, string args)
        {
            if (tuples_.ContainsKey(pragma))
            {
                throw new WixException(WixErrors.PreprocessorExtensionPragmaFailed(sourceLineNumbers, pragma, "Pragma is nested within same pragma"));
            }

            string[] untrimmed = args.Split(';');
            List<string> values = new List<string>();
            if (untrimmed != null)
            {
                foreach (string s in untrimmed)
                {
                    values.Add(s.Trim());
                }
            }
            tuples_[pragma] = values;
        }

        private void ProcessHeat(SourceLineNumberCollection sourceLineNumbers, string pragma, string args, XmlWriter writer)
        {
            string outPath = null;
            bool keepOutput = false;
            try
            {
                Assembly caller = Assembly.GetAssembly(typeof(PreprocessorExtension));
                string heatPath = caller.Location;
                heatPath = Path.GetDirectoryName(heatPath);
                heatPath = Path.Combine(heatPath, "heat.exe");
                if (!File.Exists(heatPath))
                {
                    Core.OnMessage(WixErrors.FileNotFound(sourceLineNumbers, heatPath));
                    return;
                }

                // Expand preprocessor variables
                args = Core.PreprocessString(sourceLineNumbers, args);
                Core.OnMessage(new WixGenericMessageEventArgs(sourceLineNumbers, 0, MessageLevel.Information, $"Executing heat command: \"{heatPath}\" {args}"));

                // Set temp output file
                string[] heatArgs = CommandLineToArgs(args);
                bool hasVar = false;
                if ((heatArgs != null) && (heatArgs.Length >= 2))
                {
                    for (int i = heatArgs.Length - 1; i >= 0; --i)
                    {
                        string a = heatArgs[i];
                        // If '-var' is specified, no need to set target path
                        if (a.Equals("-var", StringComparison.OrdinalIgnoreCase) || a.Equals("/var", StringComparison.OrdinalIgnoreCase))
                        {
                            hasVar = true;
                        }
                        // If '-o' is specified, keep the ouput file.
                        if ((a.Equals("-o", StringComparison.OrdinalIgnoreCase) || a.Equals("/o", StringComparison.OrdinalIgnoreCase)) && (i < (heatArgs.Length - 1)))
                        {
                            keepOutput = true;
                            outPath = heatArgs[i + 1];
                        }
                    }
                }

                // Attempt to figure out the harvest target.
                string heatTargetPath = null;
                if (!hasVar && pragma.Equals("dir", StringComparison.OrdinalIgnoreCase) && Directory.Exists(heatArgs[0]))
                {
                    heatTargetPath = heatArgs[0];
                    if (heatTargetPath.LastIndexOfAny(new char[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar }) != heatTargetPath.Length - 1)
                    {
                        heatTargetPath += Path.DirectorySeparatorChar;
                    }
                    Core.OnMessage(new WixGenericMessageEventArgs(sourceLineNumbers, 0, MessageLevel.Information, $"Will replace File/@Source and Payload/@SourceFile 'SourceDir\\' with '{heatTargetPath}'"));
                }

                if (!keepOutput)
                {
                    outPath = Path.GetTempFileName();
                    args += $" -o \"{outPath}\"";
                }

                ProcessStartInfo heatPI = new ProcessStartInfo(heatPath, $"{pragma} {args}");
                heatPI.UseShellExecute = false;
                heatPI.RedirectStandardError = true;
                heatPI.RedirectStandardOutput = true;
                heatPI.CreateNoWindow = true;
                Process heat = Process.Start(heatPI);
                heat.WaitForExit();

                string std = heat.StandardOutput.ReadToEnd();
                Core.OnMessage(new WixGenericMessageEventArgs(sourceLineNumbers, 0, MessageLevel.Information, std));

                // Error / warning
                std = heat.StandardError.ReadToEnd();
                if (heat.ExitCode == 0)
                {
                    if (!string.IsNullOrEmpty(std))
                    {
                        Core.OnMessage(new WixGenericMessageEventArgs(sourceLineNumbers, 0, MessageLevel.Warning, std));
                    }
                }
                else
                {
                    Core.OnMessage(WixErrors.PreprocessorError(sourceLineNumbers, std));
                    return;
                }

                // Copy heat-document with line number indications.
                XmlDocument doc = new XmlDocument();
                doc.Load(outPath);
                foreach (XmlNode e in doc.DocumentElement.ChildNodes)
                {
                    CopyNode(sourceLineNumbers, e, heatTargetPath, writer);
                }
            }
            catch (Exception ex)
            {
                Core.OnMessage(WixErrors.PreprocessorError(sourceLineNumbers, ex.Message));
            }
            finally
            {
                if (!keepOutput && !string.IsNullOrEmpty(outPath) && File.Exists(outPath))
                {
                    File.Delete(outPath);
                }
            }
        }

        // Copy heat-generated WiX elements, with line number indications.
        private void CopyNode(SourceLineNumberCollection sourceLineNumbers, XmlNode node, string heatTargetPath, XmlWriter writer)
        {
            switch (node.NodeType)
            {
                case XmlNodeType.Element:
                    writer.WriteProcessingInstruction(Preprocessor.LineNumberElementName, sourceLineNumbers.EncodedSourceLineNumbers);
                    writer.WriteStartElement(node.LocalName, node.NamespaceURI);
                    break;

                case XmlNodeType.CDATA:
                case XmlNodeType.Text:
                    string val = Core.PreprocessString(sourceLineNumbers, node.Value);
                    writer.WriteValue(val);
                    break;

                default:
                    node.WriteTo(writer);
                    break;
            }
            if (node.Attributes != null)
            {
                foreach (XmlAttribute a in node.Attributes)
                {
                    // For attributes- expand preprocessor variables
                    string val = Core.PreprocessString(sourceLineNumbers, a.Value);

                    // Set target path, if not already handled by -var or a transform. Match File/@Source or Payload/@SourceFile
                    if (!string.IsNullOrEmpty(heatTargetPath) 
                        && (a.LocalName.Equals("Source") && val.StartsWith("SourceDir\\")
                        && a.OwnerElement.LocalName.Equals("File") && a.OwnerElement.NamespaceURI.Equals("http://schemas.microsoft.com/wix/2006/wi"))
                        || (a.LocalName.Equals("SourceFile") && val.StartsWith("SourceDir\\")
                        && a.OwnerElement.LocalName.Equals("Payload") && a.OwnerElement.NamespaceURI.Equals("http://schemas.microsoft.com/wix/2006/wi")))
                    {
                        val = heatTargetPath + val.Substring("SourceDir\\".Length);
                    }
                    writer.WriteAttributeString(a.LocalName, a.NamespaceURI, val);
                }
            }
            if (node.ChildNodes != null)
            {
                foreach (XmlNode c in node.ChildNodes)
                {
                    CopyNode(sourceLineNumbers, c, heatTargetPath, writer);
                }
            }
            if (node.NodeType == XmlNodeType.Element)
            {
                writer.WriteEndElement();
            }
        }

        public override string EvaluateFunction(string prefix, string key, string[] args)
        {
            switch (prefix)
            {
                case "tuple":
                    return GetTupleElement(prefix, key, args);

                case "tuple_range":
                    return CreateTupleRange(prefix, key);

                default:
                    return null;
            }
        }

        private string CreateTupleRange(string prefix, string key)
        {
            int i;
            if (!tuples_.ContainsKey(key))
            {
                Core.OnMessage(WixErrors.InvalidPreprocessorVariable(null, $"{prefix}.${key}"));
                return null;
            }
            string range = "";
            for (i = 0; i < tuples_[key].Count; ++i)
            {
                if (i > 0)
                {
                    range += ";";
                }
                range += i.ToString();
            }
            return range;
        }

        private string GetTupleElement(string prefix, string key, string[] args)
        {
            int i;
            if (!tuples_.ContainsKey(key))
            {
                Core.OnMessage(WixErrors.InvalidPreprocessorVariable(null, $"{prefix}.${key}"));
                return null;
            }
            if (args.Length != 1)
            {
                Core.OnMessage(WixErrors.PreprocessorError(null, "Tuple variable function must have a single argument"));
                return null;
            }
            if (!int.TryParse(args[0], out i))
            {
                Core.OnMessage(WixErrors.PreprocessorExtensionEvaluateFunctionFailed(null, prefix, key, args[0], "Expecting an index as argument"));
                return null;
            }

            if ((i < 0) || (i >= tuples_[key].Count))
            {
                Core.OnMessage(WixErrors.PreprocessorExtensionEvaluateFunctionFailed(null, prefix, key, args[0], $"Argument out of range. Expected values 0..{tuples_[key].Count - 1}"));
                return null;
            }

            return tuples_[key][i];
        }

        private void TupleAssert(SourceLineNumberCollection sourceLineNumbers, string pragma, string args)
        {
            string[] untrimmed = args.Split(new char[] { ';' }, StringSplitOptions.RemoveEmptyEntries);
            if ((untrimmed == null) || (untrimmed.Length == 0))
            {
                Core.OnMessage(WixErrors.PreprocessorExtensionPragmaFailed(sourceLineNumbers, pragma, "Expected tuple list as argument"));
                return;
            }

            List<string> assertOn = new List<string>();
            foreach (string a in untrimmed)
            {
                string t = a.Trim();
                if (!tuples_.ContainsKey(t))
                {
                    Core.OnMessage(WixErrors.PreprocessorExtensionPragmaFailed(sourceLineNumbers, pragma, $"tuple '{t}' is undefined"));
                    return;
                }
                assertOn.Add(t);
            }

            switch (pragma)
            {
                case "EQUAL_SIZE":
                    if (assertOn.Count <= 1)
                    {
                        Core.OnMessage(WixErrors.PreprocessorExtensionPragmaFailed(sourceLineNumbers, pragma, "Expected at least two tuples in list"));
                        return;
                    }
                    int size = tuples_[assertOn[0]].Count;
                    if (! assertOn.TrueForAll((t) => tuples_[t].Count == size))
                    {
                        Core.OnMessage(WixErrors.PreprocessorExtensionPragmaFailed(sourceLineNumbers, pragma, $"Not all tuples have equal size"));
                    }
                    return;

                default:
                    return;
            }
        }

        public override string GetVariableValue(string prefix, string name)
        {
            switch (prefix)
            {
                case "tuple":
                    int i = name.LastIndexOf('.');
                    if (i <= 1)
                    {
                        return null;
                    }
                    string key = name.Substring(0, i);
                    string arg = name.Substring(i + 1);
                    return EvaluateFunction(prefix, key, new string[] { arg });

                case "tuple_range":
                    return EvaluateFunction(prefix, name, null);

                default:
                    return null;
            }
        }
    }
}