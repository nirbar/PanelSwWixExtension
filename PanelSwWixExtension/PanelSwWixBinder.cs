using Microsoft.Tools.WindowsInstallerXml;
using System;
using System.IO;

namespace PanelSw.Wix.Extensions
{
    class PanelSwWixBinder : BinderExtensionEx
    {
        public override void DatabaseAfterResolvedFields(Output output)
        {
            base.DatabaseAfterResolvedFields(output);

            ResolveTaskScheduler(output);
        }

        private void ResolveTaskScheduler(Output output)
        {
            Table taskScheduler = output.Tables["PSW_TaskScheduler"];
            if (taskScheduler == null)
            {
                return;
            }

            foreach (Row r in taskScheduler.Rows)
            {
                string xmlFile = r[3].ToString();
                if (xmlFile.Contains("!(bindpath."))
                {
                    Core.OnMessage(WixErrors.UnresolvedBindReference(null, "TaskScheduler XmlFile"));
                }
                if (!File.Exists(xmlFile))
                {
                    continue;
                }
                string xml = File.ReadAllText(xmlFile);
                xml = xml.Trim();
                xml = xml.Replace("\r", "");
                xml = xml.Replace("\n", "");
                xml = xml.Replace(Environment.NewLine, "");
                r[3] = xml;
            }
        }
    }
}