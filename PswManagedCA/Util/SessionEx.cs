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

        public static void LogObfuscated(this Session session, string msg)
        {
            string[] hiddenProps = session["MsiHiddenProperties"].Split(new char[] { ';' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (string p in hiddenProps)
            {
                msg = msg.Replace($"[{p}]", "*******");
            }
            msg = session.Format(msg);
            msg = msg.Replace("[", @"[\[]");
            session.Log(msg);
        }
    }
}
