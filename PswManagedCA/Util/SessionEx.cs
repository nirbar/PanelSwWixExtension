using Microsoft.Deployment.WindowsInstaller;
using System;

namespace PswManagedCA.Util
{
    public enum ErrorHandling
    {
        fail = 0,
        ignore = 1,
        prompt = 2
    }

    public enum PswErrorMessages
    {
        TopShelfFailure = 27000,
        ExecOnFailure = 27001,
        ServiceConfigFailure = 27002,
        DismPackageFailure = 27003,
        DismFeatureFailure = 27004,
        SqlScriptFailure = 27005,
        ExecOnConsoleFailure = 27006,
        WebsiteConfigFailure = 27007,
        SqlSearchFailure = 27008,
        JsonJpathFailure = 27009,
        DismUnwantedFeatureFailure = 27010,
        ExecOnPromptAlways = 27011,
        PromptFileDowngrades = 27012,
        ZipFileError = 27013,
        ZipArchiveError = 27014,
        UnzipArchiveError = 27015,
    }

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
            string[] hiddenProps = session["MsiHiddenProperties"]
                .Split(new char[] {';'}, StringSplitOptions.RemoveEmptyEntries);
            foreach (string p in hiddenProps)
            {
                msg = msg.Replace($"[{p}]", "******");
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

        public static MessageResult HandleError(this Session session, ErrorHandling errorHandling, int errCode, params object[] prms)
        {
            switch (errorHandling)
            {
                default: // Silent / fail
                    return MessageResult.Abort;

                case ErrorHandling.ignore:
                    return MessageResult.Ignore;

                case ErrorHandling.prompt:
                    break;
            }

            using (Record rec = new Record(2 + (prms?.Length ?? 1)))
            {
                rec[1] = errCode;
                if (prms != null)
                {
                    for (int i = 0; i < prms.Length; ++i)
                    {
                        rec[i + 2] = prms[i];
                    }
                }

                int hint = (int)InstallMessage.Error | (int)MessageButtons.AbortRetryIgnore | (int)MessageDefaultButton.Button1 | (int)MessageIcon.Error;
                return session.Message((InstallMessage)hint, rec);
            }
        }
    }
}
