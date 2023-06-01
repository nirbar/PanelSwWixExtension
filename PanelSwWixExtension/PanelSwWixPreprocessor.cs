using System;
using System.IO;
using System.Linq;
using WixToolset.Data;
using WixToolset.Extensibility.Data;
using WixToolset.Extensibility.Services;

namespace PanelSw.Wix.Extensions
{
    class PanelSwWixPreprocessor : WixToolset.Extensibility.BasePreprocessorExtension
    {
        public PanelSwWixPreprocessor()
        {
            Prefixes = new string[] { "psw" };
        }

        internal IPreprocessContext PreprocessorContext => Context;

        public override string EvaluateFunction(string prefix, string function, string[] args)
        {
            switch (function)
            {
                case "VarNullOrEmpty":
                    if (args.Length != 1 || string.IsNullOrEmpty(args[0]))
                    {
                        Messaging.Write(ErrorMessages.InvalidPreprocessorFunction(null, function));
                        break;
                    }

                    string val = PreprocessHelper.GetVariableValue(Context, "var", args[0]);
                    return string.IsNullOrEmpty(val) ? "1" : "0";

                case "AutoGuid":
                    if (args.Length == 0)
                    {
                        Messaging.Write(ErrorMessages.InvalidPreprocessorFunction(null, function));
                        break;
                    }

                    IBackendHelper backendHelper = base.Context.ServiceProvider.GetService<IBackendHelper>();
                    string key = args.Aggregate((a, c) => $"{a}\\{c}");
                    string guid = backendHelper.CreateGuid(new Guid("{F026BBCE-4776-402C-BF36-352781805165}"), key);
                    return guid;

                case "FileExists":
                    if (args.Length != 1 || string.IsNullOrEmpty(args[0]))
                    {
                        Messaging.Write(ErrorMessages.InvalidPreprocessorFunction(null, function));
                        break;
                    }

                    return File.Exists(args[0]) ? "1" : "0";

                case "DirExists":
                    if (args.Length != 1 || string.IsNullOrEmpty(args[0]))
                    {
                        Messaging.Write(ErrorMessages.InvalidPreprocessorFunction(null, function));
                        break;
                    }

                    return Directory.Exists(args[0]) ? "1" : "0";

                case "DirEmpty":
                    if (args.Length != 1 || string.IsNullOrEmpty(args[0]))
                    {
                        Messaging.Write(ErrorMessages.InvalidPreprocessorFunction(null, function));
                        break;
                    }

                    return (Directory.Exists(args[0]) && (Directory.GetFiles(args[0], "*", SearchOption.AllDirectories).Length > 0)) ? "0" : "1";
            }

            return null;
        }
    }
}
