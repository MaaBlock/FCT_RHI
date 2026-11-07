namespace FCT
{

    /**
       * @cond CHINESE
       * @brief 显示错误对话框
       * @param title 标题
       * @param message 消息内容
       * @endcond
       */
    void ShowErrorDialog(const std::string& title, const std::string& message);

    /**
     * @cond CHINESE
     * @brief 显示警告对话框
     * @param title 标题
     * @param message 消息内容
     * @endcond
     */
    void ShowWarningDialog(const std::string& title, const std::string& message);

    /**
     * @cond CHINESE
     * @brief 显示确认对话框
     * @param title 标题
     * @param message 消息内容
     * @return true表示用户点击了"是"，false表示点击了"否"
     * @endcond
     */
    bool ShowConfirmDialog(const std::string& title, const std::string& message);

    /**
     * @cond CHINESE
     * @brief 显示信息对话框
     * @param title 标题
     * @param message 消息内容
     * @endcond
     */
    void ShowInfoDialog(const std::string& title, const std::string& message);

    /**
     * @cond CHINESE
     * @brief 设置无头模式对话框重定向（启用后所有弹窗输出至控制台，不阻塞GUI）
     * @param headless 是否启用无头模式
     * @endcond
     */
    void SetHeadlessDialogMode(bool headless);

    /**
     * @cond CHINESE
     * @brief 获取当前是否处于无头对话框模式
     * @endcond
     */
    bool IsHeadlessDialogMode();

}