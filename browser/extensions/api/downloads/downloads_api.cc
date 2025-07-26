mineFilenameTimeout() { CallFilenameCallback(); }

  void CallFilenameCallback() {
    if (!filename_changed_)
      return;

    std::move(filename_changed_)
        .Run(determined_filename_,
             ConvertConflictAction(determined_conflict_action_));
  }

  void ClearPendingDeterminers() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    determined_filename_.clear();
    determined_conflict_action_ = downloads::FilenameConflictAction::kUniquify;
    determiner_ = DeterminerInfo();
    filename_changed_.Reset();
    weak_ptr_factory_.reset();
    determiners_.clear();
  }

  void DeterminerRemoved(const ExtensionId& extension_id) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    for (auto iter = determiners_.begin(); iter != determiners_.end();) {
      if (iter->extension_id == extension_id) {
        iter = determiners_.erase(iter);
      } else {
        ++iter;
      }
    }
    // If we just removed the last unreported determiner, then we need to call a
    // callback.
    CheckAllDeterminersCalled();
  }

  void AddPendingDeterminer(const ExtensionId& extension_id,
                            const base::Time& installed) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    for (auto& determiner : determiners_) {
      if (determiner.extension_id == extension_id) {
        DCHECK(false) << extension_id;
        return;
      }
    }
    determiners_.push_back(DeterminerInfo(extension_id, installed));
  }

  bool DeterminerAlreadyReported(const ExtensionId& extension_id) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    for (auto& determiner : determiners_) {
      if (determiner.extension_id == extension_id) {
        return determiner.reported;
      }
    }
    return false;
  }

  void CreatorSuggestedFilename(
      const base::FilePath& filename,
      downloads::FilenameConflictAction conflict_action) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    creator_suggested_filename_ = filename;
    creator_conflict_action_ = conflict_action;
  }

  base::FilePath creator_suggested_filename() const {
    return creator_suggested_filename_;
  }

  downloads::FilenameConflictAction creator_conflict_action() const {
    return creator_conflict_action_;
  }

  void ResetCreatorSuggestion() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    creator_suggested_filename_.clear();
    creator_conflict_action_ = downloads::FilenameConflictAction::kUniquify;
  }

  // Returns false if this |extension_id| was not expected or if this
  // |extension_id| has already reported. The caller is responsible for
  // validating |filename|.
  bool DeterminerCallback(content::BrowserContext* browser_context,
                          const ExtensionId& extension_id,
                          const base::FilePath& filename,
                          downloads::FilenameConflictAction conflict_action) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    bool found_info = false;
    for (auto& determiner : determiners_) {
      if (determiner.extension_id == extension_id) {
        found_info = true;
        if (determiner.reported)
          return false;
        determiner.reported = true;
        // Do not use filename if another determiner has already overridden the
        // filename and they take precedence. Extensions that were installed
        // later take precedence over previous extensions.
        if (!filename.empty() ||
            (conflict_action != downloads::FilenameConflictAction::kUniquify)) {
          WarningSet warnings;
          ExtensionId winner_extension_id;
          ExtensionDownloadsEventRouter::DetermineFilenameInternal(
              filename, conflict_action, determiner.extension_id,
              determiner.install_time, determiner_.extension_id,
              determiner_.install_time, &winner_extension_id,
              &determined_filename_, &determined_conflict_action_, &warnings);
          if (!warnings.empty())
            WarningService::NotifyWarningsOnUI(browser_context, warnings);
          if (winner_extension_id == determiner.extension_id)
            determiner_ = determiner;
        }
        break;
      }
    }
    if (!found_info)
      return false;
    CheckAllDeterminersCalled();
    return true;
  }

 private:
  static int determine_filename_timeout_s_;

  struct DeterminerInfo {
    DeterminerInfo();
    DeterminerInfo(const ExtensionId& e_id, const base::Time& installed);
    ~DeterminerInfo();

    ExtensionId extension_id;
    base::Time install_time;
    bool reported;
  };
  typedef std::vector<DeterminerInfo> DeterminerInfoVector;

  static const char kKey[];

  // This is safe to call even while not waiting for determiners to call back;
  // in that case, the callbacks will be null so they won't be Run.
  void CheckAllDeterminersCalled() {
    for (auto& determiner : determiners_) {
      if (!determiner.reported)
        return;
    }
    CallFilenameCallback();

    // Don't clear determiners_ immediately in case there's a second listener
    // for one of the extensions, so that DetermineFilename can return
    // kTooManyListeners. After a few seconds, DetermineFilename will return
    // kUnexpectedDeterminer instead of kTooManyListeners so that determiners_
    // doesn't keep hogging memory.
    weak_ptr_factory_ = std::make_unique<
        base::WeakPtrFactory<ExtensionDownloadsEventRouterData>>(this);
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(
            &ExtensionDownloadsEventRouterData::ClearPendingDeterminers,
            weak_ptr_factory_->GetWeakPtr()),
        base::Seconds(15));
  }

  int updated_;
  int changed_fired_;
  // Dictionary representing the current state of the download. It is cleared
  // when download completes.
  base::Value::Dict json_;

  ExtensionDownloadsEventRouter::FilenameChangedCallback filename_changed_;

  DeterminerInfoVector determiners_;

  base::FilePath creator_suggested_filename_;
  downloads::FilenameConflictAction creator_conflict_action_;
  base::FilePath determined_filename_;
  downloads::FilenameConflictAction determined_conflict_action_;
  DeterminerInfo determiner_;

  // Whether a download is complete and whether the completed download is
  // deleted.
  bool is_download_completed_;
  bool is_completed_download_deleted_;

  std::unique_ptr<base::WeakPtrFactory<ExtensionDownloadsEventRouterData>>
      weak_ptr_factory_;
};

int ExtensionDownloadsEventRouterData::determine_filename_timeout_s_ = 15;

ExtensionDownloadsEventRouterData::DeterminerInfo::DeterminerInfo(
    const ExtensionId& e_id,
    const base::Time& installed)
    : extension_id(e_id), install_time(installed), reported(false) {}

ExtensionDownloadsEventRouterData::DeterminerInfo::DeterminerInfo()
    : reported(false) {}

ExtensionDownloadsEventRouterData::DeterminerInfo::~DeterminerInfo() {}

const char ExtensionDownloadsEventRouterData::kKey[] =
    "DownloadItem ExtensionDownloadsEventRouterData";

bool OnDeterminingFilenameWillDispatchCallback(
    bool* any_determiners,
    ExtensionDownloadsEventRouterData* data,
    content::BrowserContext* browser_context,
    mojom::ContextType target_context,
    const Extension* extension,
    const base::Value::Dict* listener_filter,
    std::optional<base::Value::List>& event_args_out,
    mojom::EventFilteringInfoPtr& event_filtering_info_out) {
  *any_determiners = true;
  base::Time installed =
      ExtensionPrefs::Get(browser_context)->GetLastUpdateTime(extension->id());
  data->AddPendingDeterminer(extension->id(), installed);
  return true;
}

bool Fault(bool error, const char* message_in, std::string* message_out) {
  if (!error)
    return false;
  *message_out = message_in;
  return true;
}

bool InvalidId(DownloadItem* valid_item, std::string* message_out) {
  return Fault(!valid_item, download_extension_errors::kInvalidId, message_out);
}

bool IsDownloadDeltaField(const std::string& field) {
  return ((field == kUrlKey) || (field == kFinalUrlKey) ||
          (field == kFilenameKey) || (field == kDangerKey) ||
          (field == kMimeKey) || (field == kStartTimeKey) ||
          (field == kEndTimeKey) || (field == kStateKey) ||
          (field == kCanResumeKey) || (field == kPausedKey) ||
          (field == kErrorKey) || (field == kTotalBytesKey) ||
          (field == kFileSizeKey) || (field == kExistsKey));
}

}  // namespace

const char DownloadedByExtension::kKey[] = "DownloadItem DownloadedByExtension";

DownloadedByExtension* DownloadedByExtension::Get(
    download::DownloadItem* item) {
  base::SupportsUserData::Data* data = item->GetUserData(kKey);
  return (data == nullptr) ? nullptr
                           : static_cast<DownloadedByExtension*>(data);
}

DownloadedByExtension::DownloadedByExtension(download::DownloadItem* item,
                                             const ExtensionId& id,
                                             const std::string& name)
    : id_(id), name_(name) {
  item->SetUserData(kKey, base::WrapUnique(this));
}

DownloadsDownloadFunction::DownloadsDownloadFunction() {}

DownloadsDownloadFunction::~DownloadsDownloadFunction() {}

ExtensionFunction::ResponseAction DownloadsDownloadFunction::Run() {
  std::optional<downloads::Download::Params> params =
      downloads::Download::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  const downloads::DownloadOptions& options = params->options;
  GURL download_url(options.url);
  std::string error;
  if (Fault(!download_url.is_valid(), download_extension_errors::kInvalidURL,
            &error))
    return RespondNow(Error(std::move(error)));

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("downloads_api_run_async", R"(
        semantics {
          sender: "Downloads API"
          description:
            "This request is made when an extension makes an API call to "
            "download a file."
          trigger:
            "An API call from an extension, can be in response to user input "
            "or autonomously."
          data:
            "The extension may provide any data that it has permission to "
            "access, or is provided to it by the user."
          destination: OTHER
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting:
            "This feature cannot be disabled in settings, but disabling all "
            "extensions will prevent it."
          chrome_policy {
            ExtensionInstallBlocklist {
              ExtensionInstallBlocklist: {
                entries: '*'
              }
            }
          }
        })");
  std::unique_ptr<download::DownloadUrlParameters> download_params(
      new download::DownloadUrlParameters(
          download_url, source_process_id(),
          render_frame_host() ? render_frame_host()->GetRoutingID() : -1,
          traffic_annotation));
  base::FilePath creator_suggested_filename;
  if (options.filename) {
    // Strip "%" character as it affects environment variables.
    std::string filename;
    base::ReplaceChars(*options.filename, "%", "_", &filename);
    creator_suggested_filename = base::FilePath::FromUTF8Unsafe(filename);
    if (!net::IsSafePortableRelativePath(creator_suggested_filename)) {
      return RespondNow(Error(download_extension_errors::kInvalidFilename));
    }
  }

  if (options.save_as)
    download_params->set_prompt(*options.save_as);

  if (options.headers) {
    for (const downloads::HeaderNameValuePair& header : *options.headers) {
      if (!net::HttpUtil::IsValidHeaderName(header.name)) {
        return RespondNow(Error(download_extension_errors::kInvalidHeaderName));
      }
      if (!net::HttpUtil::IsSafeHeader(header.name, header.value)) {
        return RespondNow(
            Error(download_extension_errors::kInvalidHeaderUnsafe));
      }
      if (!net::HttpUtil::IsValidHeaderValue(header.value)) {
         return RespondNow(
            Error(download_extension_errors::kInvalidHeaderValue));
      }
      download_params->add_request_header(header.name, header.value);
    }
  }

  std::string method_string = downloads::ToString(options.method);
  if (!method_string.empty())
    download_params->set_method(method_string);
  if (options.body) {
    download_params->set_post_body(
        network::ResourceRequestBody::CreateFromBytes(options.body->data(),
                                                      options.body->size()));
  }

  download_params->set_callback(
      base::BindOnce(&DownloadsDownloadFunction::OnStarted, this,
                     creator_suggested_filename, options.conflict_action));
  // Prevent login prompts for 401/407 responses.
  download_params->set_do_not_prompt_for_login(true);
  download_params->set_download_source(download::DownloadSource::EXTENSION_API);

  DownloadManager* manager = browser_context()->GetDownloadManager();
  manager->DownloadUrl(std::move(download_params));
  RecordApiFunctions(DOWNLOADS_FUNCTION_DOWNLOAD);
  return RespondLater();
}

void DownloadsDownloadFunction::OnStarted(
    const base::FilePath& creator_suggested_filename,
    downloads::FilenameConflictAction creator_conflict_action,
    DownloadItem* item,
    download::DownloadInterruptReason interrupt_reason) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  VLOG(1) << __func__ << " " << item << " " << interrupt_reason;
  if (item) {
    DCHECK_EQ(download::DOWNLOAD_INTERRUPT_REASON_NONE, interrupt_reason);
#if !BUILDFLAG(ARKWEB_ARKWEB_EXTENSIONS)
    Respond(WithArguments(static_cast<int>(item->GetId())));
#else
    int downloadId = RespondDownlodId(item->GetGuid());
    if (downloadId != -1) {
      Respond(WithArguments(downloadId));
    }
#endif
    if (!creator_suggested_filename.empty() ||
        (creator_conflict_action !=
         downloads::FilenameConflictAction::kUniquify)) {
      ExtensionDownloadsEventRouterData* data =
          ExtensionDownloadsEventRouterData::Get(item);
      if (!data) {
        data = new ExtensionDownloadsEventRouterData(item, base::Value::Dict());
      }
      data->CreatorSuggestedFilename(creator_suggested_filename,
                                     creator_conflict_action);
    }
    new DownloadedByExtension(item, extension()->id(), extension()->name());
    item->UpdateObservers();
  } else {
    DCHECK_NE(download::DOWNLOAD_INTERRUPT_REASON_NONE, interrupt_reason);
    Respond(Error(download::DownloadInterruptReasonToString(interrupt_reason)));
  }
}

#if !BUILDFLAG(ARKWEB_ARKWEB_EXTENSIONS)
DownloadsSearchFunction::DownloadsSearchFunction() {}

DownloadsSearchFunction::~DownloadsSearchFunction() {}

ExtensionFunction::ResponseAction DownloadsSearchFunction::Run() {
  std::optional<downloads::Search::Params> params =
      downloads::Search::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  DownloadManager* manager = nullptr;
  DownloadManager* incognito_manager = nullptr;
  GetManagers(browser_context(), include_incognito_information(), &manager,
              &incognito_manager);
  ExtensionDownloadsEventRouter* router =
      DownloadCoreServiceFactory::GetForBrowserContext(
          manager->GetBrowserContext())
          ->GetExtensionEventRouter();
  router->CheckForHistoryFilesRemoval();
  if (incognito_manager) {
    ExtensionDownloadsEventRouter* incognito_router =
        DownloadCoreServiceFactory::GetForBrowserContext(
            incognito_manager->GetBrowserContext())
            ->GetExtensionEventRouter();
    incognito_router->CheckForHistoryFilesRemoval();
  }
  DownloadQuery::DownloadVector results;
  std::string error;
  RunDownloadQuery(params->query, manager, incognito_manager, &error, &results);
  if (!error.empty())
    return RespondNow(Error(std::move(error)));

  base::Value::List json_results;
  for (DownloadManager::DownloadVector::const_iterator it = results.begin();
       it != results.end(); ++it) {
    DownloadItem* download_item = *it;
    uint32_t download_id = download_item->GetId();
    bool off_record =
        ((incognito_manager != nullptr) &&
         (incognito_manager->GetDownload(download_id) != nullptr));
    Profile* profile = Profile::FromBrowserContext(browser_context());
    base::Value::Dict json_item = DownloadItemToJSON(
        *it, off_record
                 ? profile->GetPrimaryOTRProfile(/*create_if_needed=*/true)
                 : profile->GetOriginalProfile());
    json_results.Append(std::move(json_item));
  }
  RecordApiFunctions(DOWNLOADS_FUNCTION_SEARCH);
  return RespondNow(WithArguments(std::move(json_results)));
}

DownloadsPauseFunction::DownloadsPauseFunction() {}

DownloadsPauseFunction::~DownloadsPauseFunction() {}

ExtensionFunction::ResponseAction DownloadsPauseFunction::Run() {
  std::optional<downloads::Pause::Params> params =
      downloads::Pause::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  DownloadItem* download_item = GetDownload(
      browser_context(), include_incognito_information(), params->download_id);
  std::string error;
  if (InvalidId(download_item, &error) ||
      Fault(download_item->GetState() != DownloadItem::IN_PROGRESS,
            download_extension_errors::kNotInProgress, &error)) {
    return RespondNow(Error(std::move(error)));
  }
  // If the item is already paused, this is a no-op and the operation will
  // silently succeed.
  download_item->Pause();
  RecordApiFunctions(DOWNLOADS_FUNCTION_PAUSE);
  return RespondNow(NoArguments());
}

DownloadsResumeFunction::DownloadsResumeFunction() {}

DownloadsResumeFunction::~DownloadsResumeFunction() {}

ExtensionFunction::ResponseAction DownloadsResumeFunction::Run() {
  std::optional<downloads::Resume::Params> params =
      downloads::Resume::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  DownloadItem* download_item = GetDownload(
      browser_context(), include_incognito_information(), params->download_id);
  std::string error;
  if (InvalidId(download_item, &error) ||
      Fault(download_item->IsPaused() && !download_item->CanResume(),
            download_extension_errors::kNotResumable, &error)) {
    return RespondNow(Error(std::move(error)));
  }
  // Note that if the item isn't paused, this will be a no-op, and the extension
  // call will seem successful.
  download_item->Resume(user_gesture());
  RecordApiFunctions(DOWNLOADS_FUNCTION_RESUME);
  return RespondNow(NoArguments());
}

DownloadsCancelFunction::DownloadsCancelFunction() {}

DownloadsCancelFunction::~DownloadsCancelFunction() {}

ExtensionFunction::ResponseAction DownloadsCancelFunction::Run() {
  std::optional<downloads::Resume::Params> params =
      downloads::Resume::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  DownloadItem* download_item = GetDownload(
      browser_context(), include_incognito_information(), params->download_id);
  if (download_item && (download_item->GetState() == DownloadItem::IN_PROGRESS))
    download_item->Cancel(true);
  // |download_item| can be NULL if the download ID was invalid or if the
  // download is not currently active.  Either way, it's not a failure.
  RecordApiFunctions(DOWNLOADS_FUNCTION_CANCEL);
  return RespondNow(NoArguments());
}

DownloadsEraseFunction::DownloadsEraseFunction() {}

DownloadsEraseFunction::~DownloadsEraseFunction() {}

ExtensionFunction::ResponseAction DownloadsEraseFunction::Run() {
  std::optional<downloads::Erase::Params> params =
      downloads::Erase::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  DownloadManager* manager = nullptr;
  DownloadManager* incognito_manager = nullptr;
  GetManagers(browser_context(), include_incognito_information(), &manager,
              &incognito_manager);
  DownloadQuery::DownloadVector results;
  std::string error;
  RunDownloadQuery(params->query, manager, incognito_manager, &error, &results);
  if (!error.empty())
    return RespondNow(Error(std::move(error)));
  base::Value::List json_results;
  for (download::DownloadItem* result : results) {
    json_results.Append(static_cast<int>(result->GetId()));
    result->Remove();
  }
  RecordApiFunctions(DOWNLOADS_FUNCTION_ERASE);
  return RespondNow(WithArguments(std::move(json_results)));
}

DownloadsRemoveFileFunction::DownloadsRemoveFileFunction() {}

DownloadsRemoveFileFunction::~DownloadsRemoveFileFunction() {}

ExtensionFunction::ResponseAction DownloadsRemoveFileFunction::Run() {
  std::optional<downloads::RemoveFile::Params> params =
      downloads::RemoveFile::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  DownloadItem* download_item = GetDownload(
      browser_context(), include_incognito_information(), params->download_id);
  std::string error;
  if (InvalidId(download_item, &error) ||
      Fault((download_item->GetState() != DownloadItem::COMPLETE),
            download_extension_errors::kNotComplete, &error) ||
      Fault(download_item->GetFileExternallyRemoved(),
            download_extension_errors::kFileAlreadyDeleted, &error))
    return RespondNow(Error(std::move(error)));
  RecordApiFunctions(DOWNLOADS_FUNCTION_REMOVE_FILE);
  download_item->DeleteFile(
      base::BindOnce(&DownloadsRemoveFileFunction::Done, this));
  return RespondLater();
}

void DownloadsRemoveFileFunction::Done(bool success) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!success) {
    Respond(Error(download_extension_errors::kFileNotRemoved));
  } else {
    Respond(NoArguments());
  }
}

DownloadsAcceptDangerFunction::DownloadsAcceptDangerFunction() {}

DownloadsAcceptDangerFunction::~DownloadsAcceptDangerFunction() {}

DownloadsAcceptDangerFunction::OnPromptCreatedCallback*
    DownloadsAcceptDangerFunction::on_prompt_created_ = nullptr;

ExtensionFunction::ResponseAction DownloadsAcceptDangerFunction::Run() {
  std::optional<downloads::AcceptDanger::Params> params =
      downloads::AcceptDanger::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  PromptOrWait(params->download_id, 10);
  return RespondLater();
}

void DownloadsAcceptDangerFunction::PromptOrWait(int download_id, int retries) {
  DownloadItem* download_item = GetDownload(
      browser_context(), include_incognito_information(), download_id);
  content::WebContents* web_contents = dispatcher()->GetVisibleWebContents();
  std::string error;
  if (InvalidId(download_item, &error) ||
      Fault(download_item->GetState() != DownloadItem::IN_PROGRESS,
            download_extension_errors::kNotInProgress, &error) ||
      Fault(!download_item->IsDangerous(),
            download_extension_errors::kNotDangerous, &error) ||
      Fault(!web_contents, download_extension_errors::kInvisibleContext,
            &error)) {
    Respond(Error(std::move(error)));
    return;
  }
  bool visible = platform_util::IsVisible(web_contents->GetNativeView());
  if (!visible) {
    if (retries > 0) {
      base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&DownloadsAcceptDangerFunction::PromptOrWait, this,
                         download_id, retries - 1),
          base::Milliseconds(100));
      return;
    }
    Respond(Error(download_extension_errors::kInvisibleContext));
    return;
  }
  RecordApiFunctions(DOWNLOADS_FUNCTION_ACCEPT_DANGER);
  // DownloadDangerPrompt displays a modal dialog using native widgets that the
  // user must either accept or cancel. It cannot be scripted.
  DownloadDangerPrompt* prompt = DownloadDangerPrompt::Create(
      download_item, web_contents,
      base::BindOnce(&DownloadsAcceptDangerFunction::DangerPromptCallback, this,
                     download_id));
  // DownloadDangerPrompt deletes itself
  if (on_prompt_created_ && !on_prompt_created_->is_null())
    std::move(*on_prompt_created_).Run(prompt);
  // Function finishes in DangerPromptCallback().
}

void DownloadsAcceptDangerFunction::DangerPromptCallback(
    int download_id,
    DownloadDangerPrompt::Action action) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DownloadItem* download_item = GetDownload(
      browser_context(), include_incognito_information(), download_id);
  std::string error;
  if (InvalidId(download_item, &error) ||
      Fault(download_item->GetState() != DownloadItem::IN_PROGRESS,
            download_extension_errors::kNotInProgress, &error)) {
    Respond(Error(std::move(error)));
    return;
  }
  switch (action) {
    case DownloadDangerPrompt::ACCEPT:
      download_item->ValidateDangerousDownload();
      break;
    case DownloadDangerPrompt::CANCEL:
      download_item->Remove();
      break;
    case DownloadDangerPrompt::DISMISS:
      break;
  }
  Respond(NoArguments());
}

DownloadsShowFunction::DownloadsShowFunction() {}

DownloadsShowFunction::~DownloadsShowFunction() {}

ExtensionFunction::ResponseAction DownloadsShowFunction::Run() {
  std::optional<downloads::Show::Params> params =
      downloads::Show::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  DownloadItem* download_item = GetDownload(
      browser_context(), include_incognito_information(), params->download_id);
  std::string error;
  if (InvalidId(download_item, &error))
    return RespondNow(Error(std::move(error)));
  download_item->ShowDownloadInShell();
  RecordApiFunctions(DOWNLOADS_FUNCTION_SHOW);
  return RespondNow(NoArguments());
}

DownloadsShowDefaultFolderFunction::DownloadsShowDefaultFolderFunction() {}

DownloadsShowDefaultFolderFunction::~DownloadsShowDefaultFolderFunction() {}

ExtensionFunction::ResponseAction DownloadsShowDefaultFolderFunction::Run() {
  DownloadManager* manager = nullptr;
  DownloadManager* incognito_manager = nullptr;
  GetManagers(browser_context(), include_incognito_information(), &manager,
              &incognito_manager);
  platform_util::OpenItem(
      Profile::FromBrowserContext(browser_context()),
      DownloadPrefs::FromDownloadManager(manager)->DownloadPath(),
      platform_util::OPEN_FOLDER, platform_util::OpenOperationCallback());
  RecordApiFunctions(DOWNLOADS_FUNCTION_SHOW_DEFAULT_FOLDER);
  return RespondNow(NoArguments());
}

DownloadsOpenFunction::OnPromptCreatedCallback*
    DownloadsOpenFunction::on_prompt_created_cb_ = nullptr;

DownloadsOpenFunction::DownloadsOpenFunction() {}

DownloadsOpenFunction::~DownloadsOpenFunction() {}

ExtensionFunction::ResponseAction DownloadsOpenFunction::Run() {
  std::optional<downloads::Open::Params> params =
      downloads::Open::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  DownloadItem* download_item = GetDownload(
      browser_context(), include_incognito_information(), params->download_id);
  std::string error;
  if (InvalidId(download_item, &error) ||
      Fault(!user_gesture(), download_extension_errors::kUserGesture, &error) ||
      Fault(download_item->GetState() != DownloadItem::COMPLETE,
            download_extension_errors::kNotComplete, &error) ||
      Fault(download_item->GetFileExternallyRemoved(),
            download_extension_errors::kFileAlreadyDeleted, &error) ||
      Fault(!extension()->permissions_data()->HasAPIPermission(
                APIPermissionID::kDownloadsOpen),
            download_extension_errors::kOpenPermission, &error)) {
    return RespondNow(Error(std::move(error)));
  }

  WindowController* window_controller =
      ChromeExtensionFunctionDetails(this).GetCurrentWindowController();
  if (!window_controller) {
    return RespondNow(Error(download_extension_errors::kInvisibleContext));
  }
  content::WebContents* active_contents = window_controller->GetActiveTab();
  if (!active_contents) {
    return RespondNow(Error(download_extension_errors::kInvisibleContext));
  }

  // Extensions with debugger permission could fake user gestures and should
  // not be trusted.
  if (GetSenderWebContents() &&
      GetSenderWebContents()->HasRecentInteraction() &&
      !extension()->permissions_data()->HasAPIPermission(
          APIPermissionID::kDebugger)) {
    download_item->OpenDownload();
    return RespondNow(NoArguments());
  }
  // Prompt user for ack to open the download.
  // TODO(qinmin): check if user prefers to open all download using the same
  // extension, or check the recent user gesture on the originating webcontents
  // to avoid showing the prompt.
  DownloadOpenPrompt* download_open_prompt =
      DownloadOpenPrompt::CreateDownloadOpenConfirmationDialog(
          active_contents, extension()->name(), download_item->GetFullPath(),
          base::BindOnce(&DownloadsOpenFunction::OpenPromptDone, this,
                         params->download_id));
  if (on_prompt_created_cb_)
    std::move(*on_prompt_created_cb_).Run(download_open_prompt);
  RecordApiFunctions(DOWNLOADS_FUNCTION_OPEN);
  return RespondLater();
}

void DownloadsOpenFunction::OpenPromptDone(int download_id, bool accept) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::string error;
  if (Fault(!accept, download_extension_errors::kOpenPermission, &error)) {
    Respond(Error(std::move(error)));
    return;
  }
  DownloadItem* download_item = GetDownload(
      browser_context(), include_incognito_information(), download_id);
  if (Fault(!download_item, download_extension_errors::kFileAlreadyDeleted,
            &error)) {
    Respond(Error(std::move(error)));
    return;
  }
  download_item->OpenDownload();
  Respond(NoArguments());
}

DownloadsSetShelfEnabledFunction::DownloadsSetShelfEnabledFunction() {}

DownloadsSetShelfEnabledFunction::~DownloadsSetShelfEnabledFunction() {}

ExtensionFunction::ResponseAction DownloadsSetShelfEnabledFunction::Run() {
  std::optional<downloads::SetShelfEnabled::Params> params =
      downloads::SetShelfEnabled::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  // TODO(devlin): Solve this with the feature system.
  if (!extension()->permissions_data()->HasAPIPermission(
          APIPermissionID::kDownloadsShelf)) {
    return RespondNow(Error(download_extension_errors::kShelfPermission));
  }

  RecordApiFunctions(DOWNLOADS_FUNCTION_SET_SHELF_ENABLED);
  DownloadCoreService* service = nullptr;
  DownloadCoreService* incognito_service = nullptr;
  GetDownloadCoreServices(browser_context(), include_incognito_information(),
                          &service, &incognito_service);

  MaybeSetUiEnabled(service, incognito_service, extension(), params->enabled);

  for (WindowController* window : *WindowControllerList::GetInstance()) {
    DownloadCoreService* current_service =
        DownloadCoreServiceFactory::GetForBrowserContext(window->profile());
    // The following code is to hide the download UI explicitly if the UI is
    // set to disabled.
    bool match_current_service =
        (current_service == service) || (current_service == incognito_service);
    if (!match_current_service || current_service->IsDownloadUiEnabled()) {
      continue;
    }
    // Calling this API affects the download bubble as well, so extensions
    // using this API is still compatible with the new download bubble. This
    // API will eventually be deprecated (replaced by the SetUiOptions API
    // below).
    Browser* browser = window->GetBrowser();
    if (download::IsDownloadBubbleEnabled() &&
        browser->window()->GetDownloadBubbleUIController()) {
      browser->window()->GetDownloadBubbleUIController()->HideDownloadUi();
    } else if (browser->window()->IsDownloadShelfVisible()) {
      browser->window()->GetDownloadShelf()->Close();
    }
  }

  if (params->enabled &&
      ((service && !service->IsDownloadUiEnabled()) ||
       (incognito_service && !incognito_service->IsDownloadUiEnabled()))) {
    return RespondNow(Error(download_extension_errors::kShelfDisabled));
  }

  return RespondNow(NoArguments());
}

DownloadsSetUiOptionsFunction::DownloadsSetUiOptionsFunction() = default;

DownloadsSetUiOptionsFunction::~DownloadsSetUiOptionsFunction() = default;

ExtensionFunction::ResponseAction DownloadsSetUiOptionsFunction::Run() {
  std::optional<downloads::SetUiOptions::Params> params =
      downloads::SetUiOptions::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  const downloads::UiOptions& options = params->options;
  if (!extension()->permissions_data()->HasAPIPermission(
          APIPermissionID::kDownloadsUi)) {
    return RespondNow(Error(download_extension_errors::kUiPermission));
  }

  RecordApiFunctions(DOWNLOADS_FUNCTION_SET_UI_OPTIONS);
  DownloadCoreService* service = nullptr;
  DownloadCoreService* incognito_service = nullptr;
  GetDownloadCoreServices(browser_context(), include_incognito_information(),
                          &service, &incognito_service);

  MaybeSetUiEnabled(service, incognito_service, extension(), options.enabled);

  for (WindowController* window : *WindowControllerList::GetInstance()) {
    DownloadCoreService* current_service =
        DownloadCoreServiceFactory::GetForBrowserContext(window->profile());
    // The following code is to hide the download UI explicitly if the UI is
    // set to disabled.
    bool match_current_service =
        (current_service == service) || (current_service == incognito_service);
    if (!match_current_service || current_service->IsDownloadUiEnabled()) {
      continue;
    }

    Browser* browser = window->GetBrowser();
    if (download::IsDownloadBubbleEnabled() &&
        browser->window()->GetDownloadBubbleUIController()) {
      browser->window()->GetDownloadBubbleUIController()->HideDownloadUi();
    } else if (browser->window()->IsDownloadShelfVisible()) {
      browser->window()->GetDownloadShelf()->Close();
    }
  }

  if (options.enabled &&
      ((service && !service->IsDownloadUiEnabled()) ||
       (incognito_service && !incognito_service->IsDownloadUiEnabled()))) {
    return RespondNow(Error(download_extension_errors::kUiDisabled));
  }

  return RespondNow(NoArguments());
}

DownloadsGetFileIconFunction::DownloadsGetFileIconFunction()
    : icon_extractor_(new DownloadFileIconExtractorImpl()) {}

DownloadsGetFileIconFunction::~DownloadsGetFileIconFunction() {}

void DownloadsGetFileIconFunction::SetIconExtractorForTesting(
    DownloadFileIconExtractor* extractor) {
  DCHECK(extractor);
  icon_extractor_.reset(extractor);
}

ExtensionFunction::ResponseAction DownloadsGetFileIconFunction::Run() {
  std::optional<downloads::GetFileIcon::Params> params =
      downloads::GetFileIcon::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  const std::optional<downloads::GetFileIconOptions>& options = params->options;
  DownloadItem* download_item = GetDownload(
      browser_context(), include_incognito_information(), params->download_id);
  std::string error;
  if (InvalidId(download_item, &error) ||
      Fault(download_item->GetTargetFilePath().empty(),
            download_extension_errors::kEmptyFile, &error))
    return RespondNow(Error(std::move(error)));

  int icon_size = kDefaultIconSize;
  if (options && options->size) {
    icon_size = *options->size;
    if (icon_size != 16 && icon_size != 32) {
      return RespondNow(Error("Invalid `size`. Must be either `16` or `32`."));
    }
  }

  // In-progress downloads return the intermediate filename for GetFullPath()
  // which doesn't have the final extension. Therefore a good file icon can't be
  // found, so use GetTargetFilePath() instead.
  DCHECK(icon_extractor_.get());
  DCHECK(icon_size == 16 || icon_size == 32);
  float scale = 1.0;
  content::WebContents* web_contents = dispatcher()->GetVisibleWebContents();
  if (web_contents && web_contents->GetRenderWidgetHostView())
    scale = web_contents->GetRenderWidgetHostView()->GetDeviceScaleFactor();
  EXTENSION_FUNCTION_VALIDATE(icon_extractor_->ExtractIconURLForPath(
      download_item->GetTargetFilePath(), scale,
      IconLoaderSizeFromPixelSize(icon_size),
      base::BindOnce(&DownloadsGetFileIconFunction::OnIconURLExtracted, this)));
  return RespondLater();
}

void DownloadsGetFileIconFunction::OnIconURLExtracted(const std::string& url) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::string error;
  if (Fault(url.empty(), download_extension_errors::kIconNotFound, &error)) {
    Respond(Error(std::move(error)));
    return;
  }
  RecordApiFunctions(DOWNLOADS_FUNCTION_GET_FILE_ICON);
  Respond(WithArguments(url));
}
#endif

ExtensionDownloadsEventRouter::ExtensionDownloadsEventRouter(
    Profile* profile,
    DownloadManager* manager)
    : profile_(profile), notifier_(manager, this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(profile_);
  extension_registry_observation_.Observe(ExtensionRegistry::Get(profile_));
  EventRouter* router = EventRouter::Get(profile_);
  if (router)
    router->RegisterObserver(this,
                             downloads::OnDeterminingFilename::kEventName);
}

ExtensionDownloadsEventRouter::~ExtensionDownloadsEventRouter() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  EventRouter* router = EventRouter::Get(profile_);
  if (router)
    router->UnregisterObserver(this);
}

void ExtensionDownloadsEventRouter::
    SetDetermineFilenameTimeoutSecondsForTesting(int s) {
  ExtensionDownloadsEventRouterData::
      SetDetermineFilenameTimeoutSecondsForTesting(s);
}

void ExtensionDownloadsEventRouter::SetUiEnabled(const Extension* extension,
                                                 bool enabled) {
  auto iter = ui_disabling_extensions_.find(extension);
  if (iter == ui_disabling_extensions_.end()) {
    if (!enabled)
      ui_disabling_extensions_.insert(extension);
  } else if (enabled) {
    ui_disabling_extensions_.erase(extension);
  }
}

bool ExtensionDownloadsEventRouter::IsUiEnabled() const {
  return ui_disabling_extensions_.empty();
}

// The method by which extensions hook into the filename determination process
// is based on the method by which the omnibox API allows extensions to hook
// into the omnibox autocompletion process. Extensions that wish to play a part
// in the filename determination process call
// chrome.downloads.onDeterminingFilename.addListener, which adds an
// EventListener object to ExtensionEventRouter::listeners().
//
// When a download's filename is being determined, DownloadTargetDeterminer (via
// ChromeDownloadManagerDelegate (CDMD) ::NotifyExtensions()) passes a callback
// to ExtensionDownloadsEventRouter::OnDeterminingFilename (ODF), which stores
// the callback in the item's ExtensionDownloadsEventRouterData (EDERD) along
// with all of the extension IDs that are listening for onDeterminingFilename
// events. ODF dispatches chrome.downloads.onDeterminingFilename.
//
// When the extension's event handler calls |suggestCallback|,
// downloads_custom_bindings.js calls
// DownloadsInternalDetermineFilenameFunction::RunAsync, which calls
// EDER::DetermineFilename, which notifies the item's EDERD.
//
// When the last extension's event handler returns, EDERD invokes the callback
// that CDMD passed to ODF, allowing DownloadTargetDeterminer to continue the
// filename determination process. If multiple extensions wish to override the
// filename, then the extension that was last installed wins.

void ExtensionDownloadsEventRouter::OnDeterminingFilename(
    DownloadItem* item,
    const base::FilePath& suggested_path,
    FilenameChangedCallback filename_changed_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ExtensionDownloadsEventRouterData* data =
      ExtensionDownloadsEventRouterData::Get(item);
  if (!data) {
    std::move(filename_changed_callback)
        .Run({}, DownloadPathReservationTracker::UNIQUIFY);
    return;
  }
  data->BeginFilenameDetermination(std::move(filename_changed_callback));
  bool any_determiners = false;
  base::Value::Dict json = DownloadItemToJSON(item, profile_);
  json.Set(kFilenameKey, suggested_path.LossyDisplayName());
  DispatchEvent(events::DOWNLOADS_ON_DETERMINING_FILENAME,
                downloads::OnDeterminingFilename::kEventName, false,
                base::BindRepeating(&OnDeterminingFilenameWillDispatchCallback,
                                    &any_determiners, data),
                base::Value(std::move(json)));
  if (!any_determiners) {
    data->CallFilenameCallback();
    data->ClearPendingDeterminers();
    data->ResetCreatorSuggestion();
  }
}

void ExtensionDownloadsEventRouter::DetermineFilenameInternal(
    const base::FilePath& filename,
    downloads::FilenameConflictAction conflict_action,
    const ExtensionId& suggesting_extension_id,
    const base::Time& suggesting_install_time,
    const ExtensionId& incumbent_extension_id,
    const base::Time& incumbent_install_time,
    ExtensionId* winner_extension_id,
    base::FilePath* determined_filename,
    downloads::FilenameConflictAction* determined_conflict_action,
    WarningSet* warnings) {
  DCHECK(!filename.empty() ||
         (conflict_action != downloads::FilenameConflictAction::kUniquify));
  DCHECK(!suggesting_extension_id.empty());

  if (incumbent_extension_id.empty()) {
    *winner_extension_id = suggesting_extension_id;
    *determined_filename = filename;
    *determined_conflict_action = conflict_action;
    return;
  }

  if (suggesting_install_time < incumbent_install_time) {
    *winner_extension_id = incumbent_extension_id;
    warnings->insert(Warning::CreateDownloadFilenameConflictWarning(
        suggesting_extension_id, incumbent_extension_id, filename,
        *determined_filename));
    return;
  }

  *winner_extension_id = suggesting_extension_id;
  warnings->insert(Warning::CreateDownloadFilenameConflictWarning(
      incumbent_extension_id, suggesting_extension_id, *determined_filename,
      filename));
  *determined_filename = filename;
  *determined_conflict_action = conflict_action;
}

bool ExtensionDownloadsEventRouter::DetermineFilename(
    content::BrowserContext* browser_context,
    bool include_incognito,
    const ExtensionId& ext_id,
    int download_id,
    const base::FilePath& const_filename,
    downloads::FilenameConflictAction conflict_action,
    std::string* error) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RecordApiFunctions(DOWNLOADS_FUNCTION_DETERMINE_FILENAME);
  DownloadItem* item =
      GetDownload(browser_context, include_incognito, download_id);
  ExtensionDownloadsEventRouterData* data =
      item ? ExtensionDownloadsEventRouterData::Get(item) : nullptr;
  // maxListeners=1 in downloads.idl and suggestCallback in
  // downloads_custom_bindings.js should prevent duplicate DeterminerCallback
  // calls from the same renderer, but an extension may have more than one
  // renderer, so don't DCHECK(!reported).
  if (InvalidId(item, error) ||
      Fault(item->GetState() != DownloadItem::IN_PROGRESS,
            download_extension_errors::kNotInProgress, error) ||
      Fault(!data, download_extension_errors::kUnexpectedDeterminer, error) ||
      Fault(data->DeterminerAlreadyReported(ext_id),
            download_extension_errors::kTooManyListeners, error))
    return false;
  base::FilePath::StringType filename_str(const_filename.value());
  // Allow windows-style directory separators on all platforms.
  std::replace(filename_str.begin(), filename_str.end(),
               FILE_PATH_LITERAL('\\'), FILE_PATH_LITERAL('/'));
  base::FilePath filename(filename_str);
  bool valid_filename = net::IsSafePortableRelativePath(filename);
  filename =
      (valid_filename ? filename.NormalizePathSeparators() : base::FilePath());
  // If the invalid filename check is moved to before DeterminerCallback(), then
  // it will block forever waiting for this ext_id to report.
  if (Fault(!data->DeterminerCallback(browser_context, ext_id, filename,
                                      conflict_action),
            download_extension_errors::kUnexpectedDeterminer, error) ||
      Fault((!const_filename.empty() && !valid_filename),
            download_extension_errors::kInvalidFilename, error))
    return false;
  return true;
}

void ExtensionDownloadsEventRouter::OnListenerRemoved(
    const EventListenerInfo& details) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DownloadManager* manager = notifier_.GetManager();
  if (!manager)
    return;
  bool determiner_removed =
      (details.event_name == downloads::OnDeterminingFilename::kEventName);
  EventRouter* router = EventRouter::Get(profile_);
  bool any_listeners =
      router->HasEventListener(downloads::OnChanged::kEventName) ||
      router->HasEventListener(downloads::OnDeterminingFilename::kEventName);
  if (!determiner_removed && any_listeners)
    return;
  DownloadManager::DownloadVector items;
  manager->GetAllDownloads(&items);
  for (DownloadManager::DownloadVector::const_iterator iter = items.begin();
       iter != items.end(); ++iter) {
    ExtensionDownloadsEventRouterData* data =
        ExtensionDownloadsEventRouterData::Get(*iter);
    if (!data)
      continue;
    if (determiner_removed) {
      // Notify any items that may be waiting for callbacks from this
      // extension/determiner.  This will almost always be a no-op, however, it
      // is possible for an extension renderer to be unloaded while a download
      // item is waiting for a determiner. In that case, the download item
      // should proceed.
      data->DeterminerRemoved(details.extension_id);
    }
    if (!any_listeners && data->creator_suggested_filename().empty()) {
      ExtensionDownloadsEventRouterData::Remove(*iter);
    }
  }
}

// That's all the methods that have to do with filename determination. The rest
// have to do with the other, less special events.

void ExtensionDownloadsEventRouter::OnDownloadCreated(
    DownloadManager* manager,
    DownloadItem* download_item) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!ShouldExport(*download_item))
    return;

  EventRouter* router = EventRouter::Get(profile_);
  // Avoid allocating a bunch of memory in DownloadItemToJSON if it isn't going
  // to be used.
  if (!router || (!router->HasEventListener(downloads::OnCreated::kEventName) &&
                  !router->HasEventListener(downloads::OnChanged::kEventName) &&
                  !router->HasEventListener(
                      downloads::OnDeterminingFilename::kEventName))) {
    return;
  }

  // download_item->GetFileExternallyRemoved() should always return false for
  // unfinished download.
  base::Value::Dict json_item = DownloadItemToJSON(download_item, profile_);
  DispatchEvent(events::DOWNLOADS_ON_CREATED, downloads::OnCreated::kEventName,
                true, Event::WillDispatchCallback(),
                base::Value(json_item.Clone()));
  if (!ExtensionDownloadsEventRouterData::Get(download_item) &&
      (router->HasEventListener(downloads::OnChanged::kEventName) ||
       router->HasEventListener(
           downloads::OnDeterminingFilename::kEventName))) {
    new ExtensionDownloadsEventRouterData(
        download_item, download_item->GetState() == DownloadItem::COMPLETE
                           ? base::Value::Dict()
                           : std::move(json_item));
  }
}

void ExtensionDownloadsEventRouter::OnDownloadUpdated(
    DownloadManager* manager,
    DownloadItem* download_item) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  EventRouter* router = EventRouter::Get(profile_);
  ExtensionDownloadsEventRouterData* data =
      ExtensionDownloadsEventRouterData::Get(download_item);
  if (!ShouldExport(*download_item) ||
      !router->HasEventListener(downloads::OnChanged::kEventName)) {
    return;
  }
  if (!data) {
    // The download_item probably transitioned from temporary to not temporary,
    // or else an event listener was added.
    data = new ExtensionDownloadsEventRouterData(download_item,
                                                 base::Value::Dict());
  }
  base::Value::Dict new_json;
  base::Value::Dict delta;
  delta.Set(kIdKey, static_cast<int>(download_item->GetId()));
  bool changed = false;
  // For completed downloads, update can only happen when file is removed.
  if (data->is_download_completed()) {
    if (data->is_completed_download_deleted() !=
        download_item->GetFileExternallyRemoved()) {
      DCHECK(!data->is_completed_download_deleted());
      DCHECK(download_item->GetFileExternallyRemoved());
      std::string exists = kExistsKey;
      delta.SetByDottedPath(exists + ".current", false);
      delta.SetByDottedPath(exists + ".previous", true);
      changed = true;
    }
  } else {
    new_json = DownloadItemToJSON(download_item, profile_);
    std::set<std::string> new_fields;
    // For each field in the new json representation of the download_item except
    // the bytesReceived field, if the field has changed from the previous old
    // json, set the differences in the |delta| object and remember that
    // something significant changed.
    for (auto kv : new_json) {
      new_fields.insert(kv.first);
      if (IsDownloadDeltaField(kv.first)) {
        const base::Value* old_value = data->json().Find(kv.first);
        if (!old_value || kv.second != *old_value) {
          delta.SetByDottedPath(kv.first + ".current", kv.second.Clone());
          if (old_value) {
            delta.SetByDottedPath(kv.first + ".previous", old_value->Clone());
          }
          changed = true;
        }
      }
    }

    // If a field was in the previous json but is not in the new json, set the
    // difference in |delta|.
    for (auto kv : data->json()) {
      if ((new_fields.find(kv.first) == new_fields.end()) &&
          IsDownloadDeltaField(kv.first)) {
        // estimatedEndTime disappears after completion, but bytesReceived
        // stays.
        delta.SetByDottedPath(kv.first + ".previous", kv.second.Clone());
        changed = true;
      }
    }
  }

  data->set_is_download_completed(download_item->GetState() ==
                                  DownloadItem::COMPLETE);
  // download_item->GetFileExternallyRemoved() should always return false for
  // unfinished download.
  data->set_is_completed_download_deleted(
      download_item->GetFileExternallyRemoved());
  data->set_json(std::move(new_json));

  // Update the OnChangedStat and dispatch the event if something significant
  // changed. Replace the stored json with the new json.
  data->OnItemUpdated();
  if (changed) {
    DispatchEvent(events::DOWNLOADS_ON_CHANGED,
                  downloads::OnChanged::kEventName, true,
                  Event::WillDispatchCallback(), base::Value(std::move(delta)));
    data->OnChangedFired();
  }
}

void ExtensionDownloadsEventRouter::OnDownloadRemoved(
    DownloadManager* manager,
    DownloadItem* download_item) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!ShouldExport(*download_item))
    return;
  DispatchEvent(events::DOWNLOADS_ON_ERASED, downloads::OnErased::kEventName,
                true, Event::WillDispatchCallback(),
                base::Value(static_cast<int>(download_item->GetId())));
}

void ExtensionDownloadsEventRouter::DispatchEvent(
    events::HistogramValue histogram_value,
    const std::string& event_name,
    bool include_incognito,
    Event::WillDispatchCallback will_dispatch_callback,
    base::Value arg) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!EventRouter::Get(profile_))
    return;
  base::Value::List args;
  args.Append(std::move(arg));
  // The downloads system wants to share on-record events with off-record
  // extension renderers even in incognito_split_mode because that's how
  // chrome://downloads works. The "restrict_to_profile" mechanism does not
  // anticipate this, so it does not automatically prevent sharing off-record
  // events with on-record extension renderers.
  // TODO(lazyboy): When |restrict_to_browser_context| is nullptr, this will
  // broadcast events to unrelated profiles, not just incognito. Fix this
  // by introducing "include incognito" option to Event constructor.
  // https://crbug.com/726022.
  Profile* restrict_to_browser_context =
      (include_incognito && !profile_->IsOffTheRecord()) ? nullptr
                                                         : profile_.get();
  auto event =
      std::make_unique<Event>(histogram_value, event_name, std::move(args),
                              restrict_to_browser_context);
  event->will_dispatch_callback = std::move(will_dispatch_callback);
  EventRouter::Get(profile_)->BroadcastEvent(std::move(event));
}

void ExtensionDownloadsEventRouter::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UnloadedExtensionReason reason) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto iter = ui_disabling_extensions_.find(extension);
  if (iter != ui_disabling_extensions_.end())
    ui_disabling_extensions_.erase(iter);
}

void ExtensionDownloadsEventRouter::CheckForHistoryFilesRemoval() {
  static const int kFileExistenceRateLimitSeconds = 10;
  DownloadManager* manager = notifier_.GetManager();
  if (!manager)
    return;
  base::Time now(base::Time::Now());
  int delta = now.ToTimeT() - last_checked_removal_.ToTimeT();
  if (delta <= kFileExistenceRateLimitSeconds)
    return;
  last_checked_removal_ = now;
  manager->CheckForHistoryFilesRemoval();
}

}  // namespace extensions
