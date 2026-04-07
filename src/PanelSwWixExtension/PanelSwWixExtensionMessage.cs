using WixToolset.Data;

namespace PanelSw.Wix.Extensions
{
    public static class PanelSwWixErrorMessages
    {
        public static Message ExecuteCommandSequence(SourceLineNumber sourceLineNumber, string executeCommandId, string otherActionId)
        {
            return new Message(sourceLineNumber, MessageLevel.Error, (int)PswErrorId.ExecuteCommandSequence, MessageResources.ExecuteCommandSequence, executeCommandId, otherActionId);
        }
        public static Message PayloadExceedsSize(SourceLineNumber sourceLineNumber, string payloadId, long maxSize)
        {
            return new Message(sourceLineNumber, MessageLevel.Warning, (int)PswErrorId.PayloadExceedsSize, MessageResources.PayloadExceedsSize, payloadId, maxSize);
        }
        public static Message ContainerError(SourceLineNumber sourceLineNumber, string containerId, string error)
        {
            return new Message(sourceLineNumber, MessageLevel.Error, (int)PswErrorId.ContainerError, MessageResources.ContainerError, containerId, error);
        }
        public static Message PswWixAttribute(SourceLineNumber sourceLineNumber, string attribute, string element)
        {
            return new Message(sourceLineNumber, MessageLevel.Error, (int)PswErrorId.PswWixAttribute, MessageResources.PswWixAttribute, attribute, element);
        }
        public static Message MissingContainerTemplate(SourceLineNumber sourceLineNumber, string containerId)
        {
            return new Message(sourceLineNumber, MessageLevel.Error, (int)PswErrorId.MissingContainerTemplate, MessageResources.MissingContainerTemplate, containerId);
        }
        public static Message MismatchingRemoveFolderExLongPathHandling(SourceLineNumber sourceLineNumber)
        {
            return new Message(sourceLineNumber, MessageLevel.Error, (int)PswErrorId.MismatchingRemoveFolderExLongPathHandling, MessageResources.MismatchingRemoveFolderExLongPathHandling);
        }
        public static Message MissingBundleInformation(string friendlyName)
        {
            return new Message(null, MessageLevel.Error, (int)PswErrorId.MissingBundleInformation, MessageResources.MissingBundleInformation, friendlyName);
        }
        public static Message SearchPropertyNotUppercase(SourceLineNumber sourceLineNumbers, string elementName, string attributeName, string value)
        {
            return new Message(sourceLineNumbers, MessageLevel.Error, (int)PswErrorId.SearchPropertyNotUppercase, MessageResources.SearchPropertyNotUppercase, elementName, attributeName, value);
        }
        public static Message UnresolvedBindReference(SourceLineNumber sourceLineNumbers, string BindRef)
        {
            return new Message(sourceLineNumbers, MessageLevel.Error, (int)PswErrorId.UnresolvedBindReference, MessageResources.UnresolvedBindReference, BindRef);
        }
    }

    public static class PanelSwWixWarningMessages
    {
        public static Message FileGlobNoFiles(SourceLineNumber sourceLineNumber)
        {
            return new Message(sourceLineNumber, MessageLevel.Warning, (int)PswWarningId.FileGlobNoFiles, MessageResources.FileGlobNoFiles);
        }
    }

    public enum PswErrorId : int
    {
        ExecuteCommandSequence = 9000,
        PayloadExceedsSize,
        ContainerError,
        PswWixAttribute,
        MissingContainerTemplate,
        MismatchingRemoveFolderExLongPathHandling,
        MissingBundleInformation,
        SearchPropertyNotUppercase,
        UnresolvedBindReference,
    }

    public enum PswWarningId : int
    {
        FileGlobNoFiles = 9500,
    }
}
