using System;
using System.IO;
using System.Linq;
/*
namespace PanelSw.Wix.Extensions
{
    class PanelSwWixPreprocessor : WixToolset.Extensibility.BasePreprocessorExtension
    {
        public override string[] Prefixes => new string[] { "psw" };

        public override string EvaluateFunction(string prefix, string function, string[] args)
        {
            switch (function)
            {
                case "VarNullOrEmpty":
                    if (args.Length != 1 || string.IsNullOrEmpty(args[0]))
                    {
                        throw new WixException(WixErrors.InvalidPreprocessorFunction(null, function));
                    }

                    string val = Core.GetVariableValue(null, args[0], true);
                    return string.IsNullOrEmpty(val) ? "1" : "0";

                case "AutoGuid":
                    if (args.Length == 0)
                    {
                        throw new WixException(WixErrors.InvalidPreprocessorFunction(null, function));
                    }

                    string key = args.Aggregate((a, c) => $"{a}\\{c}");
                    string guid = CompilerCore.NewGuid(new Guid("{F026BBCE-4776-402C-BF36-352781805165}"), key);
                    return guid;

                case "FileExists":
                    if (args.Length != 1 || string.IsNullOrEmpty(args[0]))
                    {
                        throw new WixException(WixErrors.InvalidPreprocessorFunction(null, function));
                    }

                    return File.Exists(args[0]) ? "1" : "0";

                case "DirExists":
                    if (args.Length != 1 || string.IsNullOrEmpty(args[0]))
                    {
                        throw new WixException(WixErrors.InvalidPreprocessorFunction(null, function));
                    }

                    return Directory.Exists(args[0]) ? "1" : "0";

                case "DirEmpty":
                    if (args.Length != 1 || string.IsNullOrEmpty(args[0]))
                    {
                        throw new WixException(WixErrors.InvalidPreprocessorFunction(null, function));
                    }

                    return (Directory.Exists(args[0]) && (Directory.GetFiles(args[0], "*", SearchOption.AllDirectories).Length > 0)) ? "0" : "1";

                default:
                    return null;
            }
        }
    }
}
*/
