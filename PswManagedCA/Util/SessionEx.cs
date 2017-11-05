using Microsoft.Deployment.WindowsInstaller;

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
    }
}
