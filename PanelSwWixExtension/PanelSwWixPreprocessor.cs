using Microsoft.Tools.WindowsInstallerXml;

namespace PanelSw.Wix.Extensions
{
    class PanelSwWixPreprocessor : PreprocessorExtension
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

                default:
                    return null;
            }
        }
    }
}