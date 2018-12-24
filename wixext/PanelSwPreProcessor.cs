using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text;
using System.Xml;
using Microsoft.Tools.WindowsInstallerXml;

namespace PanelSw.Wix.Extensions
{
    class PanelSwPreProcessor : PreprocessorExtension
    {
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
                    return true;

                case "endtuple":
                    if (!tuples_.Remove(pragma))
                    {
                        throw new WixException(WixErrors.PreprocessorExtensionPragmaFailed(sourceLineNumbers, pragma, $"endtuple pragma for undefined tuple"));
                    }
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

        private void ProcessHeat(SourceLineNumberCollection sourceLineNumbers, string pragma, string args, XmlWriter writer)
        {
            string outPath = null;
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
                outPath = Path.GetTempFileName();
                args += $" -o \"{outPath}\"";

                ProcessStartInfo heatArgs = new ProcessStartInfo(heatPath, $"{pragma} {args}");
                heatArgs.UseShellExecute = false;
                heatArgs.RedirectStandardError = true;
                heatArgs.RedirectStandardOutput = true;
                heatArgs.CreateNoWindow = true;
                Process heat = Process.Start(heatArgs);
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
                    CopyNode(sourceLineNumbers, e, writer);
                }
            }
            catch (Exception ex)
            {
                Core.OnMessage(WixErrors.PreprocessorError(sourceLineNumbers, ex.Message));
            }
            finally
            {
                if (!string.IsNullOrEmpty(outPath) && File.Exists(outPath))
                {
                    File.Delete(outPath);
                }
            }
        }

        // Copy heat-generated WiX elements, with line number indications.
        private void CopyNode(SourceLineNumberCollection sourceLineNumbers, XmlNode node, XmlWriter writer)
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
                    writer.WriteAttributeString(a.LocalName, a.NamespaceURI, val);
                }
            }
            if (node.ChildNodes != null)
            {
                foreach (XmlNode c in node.ChildNodes)
                {
                    CopyNode(sourceLineNumbers, c, writer);
                }
            }
            if (node.NodeType == XmlNodeType.Element)
            {
                writer.WriteEndElement();
            }
        }

        public override string EvaluateFunction(string prefix, string key, string[] args)
        {
            int i;
            switch (prefix)
            {
                case "tuple":
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

                case "tuple_range":
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

                default:
                    return null;
            }
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