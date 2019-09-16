using Microsoft.Deployment.WindowsInstaller;
using System;

namespace PswManagedCA.Util
{
    static class SessionEx
    {
        public static ComponentInfo ComponentByFileId(this Session session, string fileId)
        {
            using (View view = session.Database.OpenView("SELECT `Component_` FROM `File` WHERE `File`='{0}'", fileId))
            {
                view.Execute(null);
                foreach (Record rec in view)
                {
                    using (rec)
                    {
                        string comp = rec[1] as string;

                        return session.Components[comp];
                    }
                }
            }

            return null;
        }

        public static string Obfuscate(this Session session, string msg)
        {
            string[] hiddenProps = session["MsiHiddenProperties"].Split(new char[] { ';' }, StringSplitOptions.RemoveEmptyEntries);
            if (hiddenProps != null)
            {
                foreach (string p in hiddenProps)
                {
                    msg = msg.Replace($"[{p}]", "******");
                }
            }
            return session.Format(msg);
        }

        public static void LogObfuscated(this Session session, string msg)
        {
            msg = session.Obfuscate(msg);
            session.LogUnformatted(msg);
        }

        public static void LogUnformatted(this Session session, string msg)
        {
            using (Record rec = new Record(1))
            {
                rec.FormatString = "[1]";
                rec[1] = msg;
                session.Message(InstallMessage.Info, rec);
            }
        }
    }
}
