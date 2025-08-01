#include "core/stdafx.h"
#include "options/auto_pin_options.h"
#include "core/application.h"
#include "pin/auto_pin_manager.h"
#include "window/window_monitor.h"
#include "window/window_helper.h"
#include "system/language_manager.h"


// 静态控件子类，显示一个可以开/关切换的图标。
//
class IconCtl : ::noncopyable
{
public:
    enum {
        WM_SHOWICON = WM_USER,  // wparam: bool on/off
    };

    static IconCtl* subclass(HWND wnd, HICON icon)
    {
        WNDPROC orgProc = WNDPROC(GetWindowLongPtr(wnd, GWLP_WNDPROC));
        if (!orgProc)
            return nullptr;
        
        // 由于构造函数是私有的，我们不能使用make_unique
        // 但我们可以确保在失败时正确清理
        IconCtl* obj = new IconCtl();
        obj->orgProc = orgProc;
        obj->icon = icon;
        obj->show = true;
        
        SetWindowLongPtr(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(obj));
        SetWindowLongPtr(wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndProc));
        return obj;
    }

private:
    WNDPROC orgProc;
    HICON   icon;
    bool    show;

    IconCtl() {}

    static BOOL CALLBACK wndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        IconCtl* obj = reinterpret_cast<IconCtl*>(GetWindowLongPtr(wnd, GWLP_USERDATA));
        if (!obj)
            return DefWindowProc(wnd, msg, wparam, lparam) != 0;

        switch (msg) {
            case WM_NCDESTROY: {
                WNDPROC orgProc = obj->orgProc;
                delete obj;
                return CallWindowProc(orgProc, wnd, msg, wparam, lparam) != 0;
            }
            case WM_PAINT: {
                LRESULT ret = CallWindowProc(obj->orgProc, wnd, msg, wparam, lparam);
                HDC dc = GetDC(wnd);
                if (dc) {
                    if (obj->show) {
                        RECT rc;
                        GetClientRect(wnd, &rc);
                        int x = (rc.right  - GetSystemMetrics(SM_CXCURSOR)) / 2;
                        int y = (rc.bottom - GetSystemMetrics(SM_CYCURSOR)) / 2;          
                        DrawIcon(dc, x+1, y+1, obj->icon);
                    }
                    ReleaseDC(wnd, dc);
                }
                return ret != 0;
            }
            case WM_SHOWICON: {
                obj->show = wparam != 0;
                InvalidateRect(wnd, 0, true);
                UpdateWindow(wnd);
                return 0;
            }
        }

        return CallWindowProc(obj->orgProc, wnd, msg, wparam, lparam) != 0;
    }

};


namespace {
    BOOL CALLBACK apEditRuleDlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        static int tracking = 0;    // 0: none, 1: title, 2: class
        static HWND hitWnd = nullptr;
        static SIZE minSize;
        static SIZE listMarg;
        static int  btnMarg;
        static HWND tooltip = nullptr;

        switch (msg) {
            case WM_INITDIALOG: {
                if (!lparam) {
                    EndDialog(wnd, -1);
                    return true;
                }
                SetWindowLongPtr(wnd, GWLP_USERDATA, lparam);
                AutoPinRule& rule = *reinterpret_cast<AutoPinRule*>(lparam);

                // 本地化对话框文本
                Util::Dialog::localizeDialog(wnd, L"edit_rule");

                hitWnd = nullptr;

                SetDlgItemText(wnd, IDC_DESCR, rule.descr.c_str());
                SetDlgItemText(wnd, IDC_TITLE, rule.ttl.c_str());
                SetDlgItemText(wnd, IDC_CLASS, rule.cls.c_str());

                HICON target = LoadCursor(app.inst, MAKEINTRESOURCE(IDC_BULLSEYE));
                IconCtl::subclass(GetDlgItem(wnd, IDC_TTLPICK), target);
                IconCtl::subclass(GetDlgItem(wnd, IDC_CLSPICK), target);

                // 为目标图标创建工具提示
                tooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, L"",
                    WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    wnd, nullptr, app.inst, nullptr);

                TOOLINFO ti;
                ti.cbSize = sizeof(ti);
                ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
                ti.hwnd = wnd;
                ti.hinst = app.inst;
                ti.lParam = 0;

                ti.uId = UINT_PTR(GetDlgItem(wnd, IDC_TTLPICK));
                std::wstring titleTooltip = LANG_MGR.getString(L"tooltips.drag_title_tooltip");
                ti.lpszText = const_cast<LPWSTR>(titleTooltip.c_str());
                SendMessage(tooltip, TTM_ADDTOOL, 0, LPARAM(&ti));

                ti.uId = UINT_PTR(GetDlgItem(wnd, IDC_CLSPICK));
                std::wstring classTooltip = LANG_MGR.getString(L"tooltips.drag_class_tooltip");
                ti.lpszText = const_cast<LPWSTR>(classTooltip.c_str());
                SendMessage(tooltip, TTM_ADDTOOL, 0, LPARAM(&ti));

                return true;
            }
            case WM_DESTROY: {
                DestroyWindow(tooltip);
                return true;
            }
            case WM_MOUSEMOVE: {
                if (tracking) {
                    POINT pt = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                    ClientToScreen(wnd, &pt);
                    HWND newWnd = Window::getNonChildParent(WindowFromPoint(pt));
                    if (hitWnd != newWnd && !Window::isProgManWnd(newWnd) && !Window::isTaskBar(newWnd)) {
                        if (hitWnd)
                            Graphics::markWindow(hitWnd, false);

                        hitWnd = newWnd;

                        if (tracking == 1) {
                            Window::WndHelper w(GetDlgItem(wnd, IDC_TITLE));
                            w.setText(Window::WndHelper(hitWnd).getText());
                            w.update();
                        }
                        else {
                            Window::WndHelper w(GetDlgItem(wnd, IDC_CLASS));
                            w.setText(Window::WndHelper(hitWnd).getClassName());
                            w.update();
                        }

                        Graphics::markWindow(hitWnd, true);

                    }
                }
                return true;
            }
            case WM_LBUTTONUP: {
                if (tracking) {
                    if (hitWnd)
                        Graphics::markWindow(hitWnd, false);
                    ReleaseCapture();
                    SendDlgItemMessage(wnd, tracking == 1 ? IDC_TTLPICK : IDC_CLSPICK, 
                        IconCtl::WM_SHOWICON, true, 0);
                    tracking = 0;
                    hitWnd = 0;
                }
                return true;
            }
            case WM_HELP: {
                // 帮助功能已移除
                return true;
            }
            case WM_COMMAND: {
                int id = LOWORD(wparam);
                switch (id) {
                    case IDOK: {
                        AutoPinRule& rule = 
                            *reinterpret_cast<AutoPinRule*>(GetWindowLongPtr(wnd, GWLP_USERDATA));
                        WCHAR buf[Constants::MAX_CLASSNAME_LEN];
                        GetDlgItemText(wnd, IDC_DESCR, buf, sizeof(buf));  rule.descr = buf;
                        GetDlgItemText(wnd, IDC_TITLE, buf, sizeof(buf));  rule.ttl = buf;
                        GetDlgItemText(wnd, IDC_CLASS, buf, sizeof(buf));  rule.cls = buf;
                        // allow empty strings (at least title can be empty...)
                        //if (rule.ttl.empty()) rule.ttl = "*";
                        //if (rule.cls.empty()) rule.cls = "*";          
                    }
                    case IDCANCEL:
                        EndDialog(wnd, id);
                        return true;
                    case IDC_TTLPICK:
                    case IDC_CLSPICK: {
                        if (HIWORD(wparam) == STN_CLICKED) {
                            SendDlgItemMessage(wnd, id, IconCtl::WM_SHOWICON, false, 0);
                            SetCapture(wnd);
                            SetCursor(LoadCursor(app.inst, MAKEINTRESOURCE(IDC_BULLSEYE)));
                            tracking = id == IDC_TTLPICK ? 1 : 2;
                        }
                        return true;
                    }
                    default: {
                        return false;
                    }
                }
            }
            default:
                return false;
        }
    }
}


// 处理与列表控件交互的实用类。
//
class RulesList {
public:
    RulesList();

    void init(HWND wnd);
    void term();

    void setAll(const AutoPinRules& rules);
    void getAll(AutoPinRules& rules);
    void remove(std::vector<int> indices);

    int  addNew();
    bool getRule(int i, AutoPinRule& rule) const;
    bool setRule(int i, const AutoPinRule& rule);

    void selectSingleRule(int i);
    int  getFirstSel() const;
    std::vector<int> getSel() const;
    void setItemSel(int i, bool sel);
    bool getItemSel(int i) const;

    void moveSelUp();
    void moveSelDown();

    operator HWND() const
    {
        return list;
    }

    void checkClick(HWND page)
    {
        DWORD xy = GetMessagePos();
        LVHITTESTINFO hti;
        hti.pt.x = GET_X_LPARAM(xy);
        hti.pt.y = GET_Y_LPARAM(xy);
        ScreenToClient(list, &hti.pt);
        int i = ListView_HitTest(list, &hti);
        if (i >= 0 && hti.flags & LVHT_ONITEMSTATEICON) {
            Window::psChanged(page);
        }
    }

protected:
    HWND list;
    std::wstring m_columnHeaderText; // 存储本地化的列标题文本

    AutoPinRule* getData(int i) const;
    bool setData(int i, AutoPinRule* rule);

};


RulesList::RulesList() : list(nullptr)
{
}


void RulesList::init(HWND wnd)
{
    list = wnd;
    ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES);
    ListView_SetCallbackMask(list, LVIS_STATEIMAGEMASK);

    // 调整列宽度以适应客户区宽度
    RECT rc;
    GetClientRect(list, &rc);    
    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt  = LVCFMT_LEFT;
    lvc.cx   = rc.right - rc.left;
    
    // 获取本地化的列标题文本
    m_columnHeaderText = LANG_MGR.getString(L"dialogs.autopin.rules");
    
    // 移除末尾的冒号（如果存在）
    if (!m_columnHeaderText.empty() && m_columnHeaderText.back() == L':') {
        m_columnHeaderText.pop_back();
    }
    
    // 如果获取失败，使用英文回退
    if (m_columnHeaderText.empty() || m_columnHeaderText == L"dialogs.autopin.rules") {
        m_columnHeaderText = LANG_MGR.getString(L"autopin.rules");
        if (m_columnHeaderText.empty() || m_columnHeaderText == L"autopin.rules") {
            m_columnHeaderText = L"Rules";
        }
    }
    
    lvc.pszText = const_cast<LPWSTR>(m_columnHeaderText.c_str());
    ListView_InsertColumn(list, 0, LPARAM(&lvc));
}


void RulesList::term()
{
    list = nullptr;
}


// set all the rules to the list
//
void RulesList::setAll(const AutoPinRules& rules)
{
    for (int n = 0; n < int(rules.size()); ++n) {
        int i = addNew();
        if (i < 0)
            continue;
        AutoPinRule* rule = getData(i);
        if (!rule)
            continue;
        *rule = rules[n];
    }
}


// get all the rules from the list
//
void RulesList::getAll(AutoPinRules& rules)
{
    rules.clear();

    int cnt = ListView_GetItemCount(list);
    for (int n = 0; n < cnt; ++n) {
        const AutoPinRule* rule = getData(n);
        if (!rule)
            continue;
        rules.push_back(*rule);
    }
}


void RulesList::remove(std::vector<int> indices)
{
    for (int n = static_cast<int>(indices.size())-1; n >= 0; --n)
        ListView_DeleteItem(list, indices[n]);
}


// add a new, empty list item (with a valid data object)
//
int RulesList::addNew()
{
    // 获取本地化的新规则描述文本
    std::wstring newRuleDesc = LANG_MGR.getString(L"strings.new_rule_description");
    if (newRuleDesc.empty() || newRuleDesc == L"strings.new_rule_description") {
        // 回退到英文
        newRuleDesc = L"<New>";
    }
    
    // 使用智能指针管理内存，并传入本地化的描述文本
    std::unique_ptr<AutoPinRule> rule = std::make_unique<AutoPinRule>(newRuleDesc);

    LVITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = ListView_GetItemCount(list);
    lvi.iSubItem = 0;
    lvi.pszText = LPSTR_TEXTCALLBACK;
    lvi.lParam = LPARAM(rule.get());
    int i = ListView_InsertItem(list, &lvi);
    if (i >= 0) {
        rule.release(); // 成功插入，释放所有权给列表控件
    }
    // 如果插入失败，智能指针会自动清理内存

    return i;
}


// get the data object of an item
//
AutoPinRule* RulesList::getData(int i) const
{
    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = i;
    lvi.iSubItem = 0;
    return ListView_GetItem(list, &lvi) ? (AutoPinRule*)lvi.lParam : nullptr;
}


// set the data object of an item
//
bool RulesList::setData(int i, AutoPinRule* rule)
{
    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = i;
    lvi.iSubItem = 0;
    lvi.lParam = LPARAM(rule);
    return !!ListView_SetItem(list, &lvi);
}


// get a rule from the list
//
bool RulesList::getRule(int i, AutoPinRule& rule) const
{
    const AutoPinRule* r = getData(i);
    if (!r)
        return false;
    rule = *r;
    return true;
}


// set a rule to the list
//
bool RulesList::setRule(int i, const AutoPinRule& rule)
{
    AutoPinRule* r = getData(i);
    if (!r)
        return false;
    *r = rule;
    ListView_Update(list, i);
    // use this to update the focus rect size
    ListView_SetItemText(list, i, 0, LPSTR_TEXTCALLBACK);
    return true;
}


// select (highlight) an item and de-select the rest
//
void RulesList::selectSingleRule(int i)
{
    int cnt = ListView_GetItemCount(list);
    for (int n = 0; n < cnt; ++n)
        ListView_SetItemState(list, n, n==i ? LVIS_SELECTED : 0, LVIS_SELECTED);
}


// get index of first selected item
// (or -1 if none are selected)
//
int RulesList::getFirstSel() const
{
    int cnt = ListView_GetItemCount(list);
    for (int n = 0; n < cnt; ++n) {
        UINT state = ListView_GetItemState(list, n, LVIS_SELECTED);
        if (state & LVIS_SELECTED)
            return n;
    }
    return -1;
}


// get indices of all selected items
//
std::vector<int> RulesList::getSel() const
{
    std::vector<int> ret;
    int cnt = ListView_GetItemCount(list);
    for (int n = 0; n < cnt; ++n) {
        UINT state = ListView_GetItemState(list, n, LVIS_SELECTED);
        if (state & LVIS_SELECTED)
            ret.push_back(n);
    }
    return ret;
}


void RulesList::setItemSel(int i, bool sel)
{
    UINT state = ListView_GetItemState(list, i, LVIS_SELECTED);
    bool curSel = (state & LVIS_SELECTED) != 0;
    if (curSel != sel)
        ListView_SetItemState(list, i, sel ? LVIS_SELECTED : 0, LVIS_SELECTED);
}


bool RulesList::getItemSel(int i) const
{
    UINT state = ListView_GetItemState(list, i, LVIS_SELECTED);
    return (state & LVIS_SELECTED) != 0;
}


void RulesList::moveSelUp()
{
    int cnt = ListView_GetItemCount(list);
    std::vector<int> sel = getSel();
    // nop if:
    // - nothing's selected
    // - everything's selected
    // - the first item is selected
    if (!sel.size() || sel.size() == cnt || sel.front() == 0)
        return;

    // swap each selected item with the one before it
    for (int n = 0; n < int(sel.size()); ++n) {
        int i = sel[n];
        // move data
        AutoPinRule* tmp = getData(i-1);
        setData(i-1, getData(i));
        setData(i,   tmp);
        // move selection
        setItemSel(i-1, true);
        setItemSel(i,   false);
        // notify for text change
        ListView_SetItemText(list, i-1, 0, LPSTR_TEXTCALLBACK);
        ListView_SetItemText(list, i, 0, LPSTR_TEXTCALLBACK);
    }

}


void RulesList::moveSelDown()
{
    int cnt = ListView_GetItemCount(list);
    std::vector<int> sel = getSel();
    // nop if:
    // - nothing's selected
    // - everything's selected
    // - the last item is selected
    if (!sel.size() || sel.size() == cnt || sel.back() == cnt-1)
        return;

    // swap each selected item with the one after it
    for (int n = static_cast<int>(sel.size())-1; n >= 0 ; --n) {
        int i = sel[n];
        // move data
        AutoPinRule* tmp = getData(i+1);
        setData(i+1, getData(i));
        setData(i,   tmp);
        // move selection
        setItemSel(i+1, true);
        setItemSel(i,   false);
        // notify for text change
        ListView_SetItemText(list, i+1, 0, LPSTR_TEXTCALLBACK);
        ListView_SetItemText(list, i, 0, LPSTR_TEXTCALLBACK);
    }

}


static RulesList rlist;


// update controls state, depending on current selection
//
void OptAutoPin::uiUpdate(HWND wnd)
{
    int cnt = ListView_GetItemCount(rlist);
    int selCnt = ListView_GetSelectedCount(rlist);
    EnableWindow(GetDlgItem(wnd, IDC_EDIT),   selCnt == 1);
    EnableWindow(GetDlgItem(wnd, IDC_REMOVE), selCnt > 0);
    EnableWindow(GetDlgItem(wnd, IDC_UP),     selCnt > 0 && 
        !rlist.getItemSel(0));
    EnableWindow(GetDlgItem(wnd, IDC_DOWN),   selCnt > 0 && 
        !rlist.getItemSel(cnt-1));
}


bool OptAutoPin::cmAutoPinOn(HWND wnd)
{
    bool b = IsDlgButtonChecked(wnd, IDC_AUTOPIN_ON) == BST_CHECKED;
    Window::enableGroup(wnd, IDC_AUTOPIN_GROUP, b);

     // if turned on, disable some buttons
     if (b) uiUpdate(wnd);

     Window::psChanged(wnd);
    return true;
}


bool OptAutoPin::evInitDialog(HWND wnd, HWND focus, LPARAM param)
{
    // must have a valid data ptr
    if (!param) {
        EndDialog(wnd, IDCANCEL);
        return false;
    }

    // save the data ptr
    PROPSHEETPAGE& psp = *reinterpret_cast<PROPSHEETPAGE*>(param);
    Options& opt = reinterpret_cast<OptionsPropSheetData*>(psp.lParam)->opt;
    SetWindowLongPtr(wnd, GWLP_USERDATA, psp.lParam);

    // 获取当前DPI并现代化对话框
     int currentDpi = Graphics::DpiManager::getDpiForWindow(wnd);
     Util::Dialog::modernizeDialog(wnd, currentDpi);

     // 本地化对话框文本
     Util::Dialog::localizeDialog(wnd, L"autopin");

    CheckDlgButton(wnd, IDC_AUTOPIN_ON, opt.autoPinOn);
    cmAutoPinOn(wnd);  // simulate check to setup other ctrls

    SendDlgItemMessage(wnd, IDC_RULE_DELAY_UD, UDM_SETRANGE, 0, 
        MAKELONG(opt.autoPinDelay.maxV,opt.autoPinDelay.minV));
    SendDlgItemMessage(wnd, IDC_RULE_DELAY_UD, UDM_SETPOS, 0, 
        MAKELONG(opt.autoPinDelay.value,0));


    rlist.init(GetDlgItem(wnd, IDC_LIST));
    rlist.setAll(opt.autoPinRules);

    uiUpdate(wnd);

    return false;
}


void OptAutoPin::apply(HWND wnd, WindowCreationMonitor& winCreMon)
{
    Options& opt = reinterpret_cast<OptionsPropSheetData*>(GetWindowLongPtr(wnd, GWLP_USERDATA))->opt;

    bool autoPinOn = IsDlgButtonChecked(wnd, IDC_AUTOPIN_ON) == BST_CHECKED;
    if (opt.autoPinOn != autoPinOn) {
        if (!autoPinOn)
            winCreMon.term();
        else if (!winCreMon.init(app.mainWnd, App::WM_QUEUEWINDOW)) {
            Foundation::ErrorHandler::error(wnd, LanguageManager::getInstance().getString(L"hook_dll_error").c_str());
            autoPinOn = false;
        }
    }

    opt.autoPinDelay.value = opt.autoPinDelay.getUI(wnd, IDC_RULE_DELAY);

    if (opt.autoPinOn = autoPinOn)
        SetTimer(app.mainWnd, App::TIMERID_AUTOPIN, opt.autoPinDelay.value, 0);
    else
        KillTimer(app.mainWnd, App::TIMERID_AUTOPIN);

    rlist.getAll(opt.autoPinRules);
    
    // 立即保存设置到INI文件
    opt.saveImmediately();
}


bool OptAutoPin::validate(HWND wnd)
{
    Options& opt = reinterpret_cast<OptionsPropSheetData*>(GetWindowLongPtr(wnd, GWLP_USERDATA))->opt;
    return opt.autoPinDelay.validateUI(wnd, IDC_RULE_DELAY, true);
}


BOOL CALLBACK OptAutoPin::dlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
        case WM_INITDIALOG: {
            return evInitDialog(wnd, HWND(wparam), lparam);
        }
        case WM_DESTROY: {
            rlist.term();
            return true;
        }
        case WM_NOTIFY: {
            NMHDR nmhdr = *reinterpret_cast<NMHDR*>(lparam);
            switch (nmhdr.code) {
            case PSN_SETACTIVE: {
                HWND tab = PropSheet_GetTabControl(nmhdr.hwndFrom);
                app.optionPage = static_cast<int>(SendMessage(tab, TCM_GETCURSEL, 0, 0));
                SetWindowLongPtr(wnd, DWLP_MSGRESULT, 0);
                return true;
            }
            case PSN_KILLACTIVE: {
                SetWindowLongPtr(wnd, DWLP_MSGRESULT, !validate(wnd));
                return true;
            }
            case PSN_APPLY: {
                WindowCreationMonitor& winCreMon = reinterpret_cast<OptionsPropSheetData*>(GetWindowLongPtr(wnd, GWLP_USERDATA))->winCreMon;
                apply(wnd, winCreMon);
                return true;
            }
            case UDN_DELTAPOS: {
                if (wparam == IDC_RULE_DELAY_UD) {
                    NM_UPDOWN& nmud = *(NM_UPDOWN*)lparam;
                    Options& opt = 
                        reinterpret_cast<OptionsPropSheetData*>(GetWindowLongPtr(wnd, GWLP_USERDATA))->opt;
                    nmud.iDelta *= opt.autoPinDelay.step;
                    SetWindowLongPtr(wnd, DWLP_MSGRESULT, FALSE);   // allow change
                    return true;
                }
                else
                    return false;
            }
            case LVN_ITEMCHANGED: {
                NMLISTVIEW& nmlv = *(NMLISTVIEW*)lparam;
                if (nmlv.uChanged == LVIF_STATE) uiUpdate(wnd);
                return true;
            }
            case NM_CLICK: {
                rlist.checkClick(wnd);
                return true;
            }
            case NM_DBLCLK: {
                int i = ListView_GetNextItem(rlist, -1, LVNI_FOCUSED);
                if (i != -1) {
                    // If an item has been focused and then we click on the empty area
                    // below the last item, we'll still get NM_DBLCLK.
                    // So, we must also check if the focused item is also selected.
                    if (rlist.getItemSel(i)) {
                        // we can't just send ourselves a IDC_EDIT cmd
                        // because there may be more selected items
                        // (if the CTRL is held)
                        AutoPinRule rule;
                        if (rlist.getRule(i, rule)) {
                            int res = Util::Res::LocalizedDialogBoxParam(WORD(IDD_EDIT_AUTOPIN_RULE), 
                                 wnd, (DLGPROC)apEditRuleDlgProc, LPARAM(&rule));
                             if (res == IDOK) {
                                 rlist.setRule(i, rule);
                                 Window::psChanged(wnd);
                             }
                        }
                    }
                }
                return true;
            }
            case LVN_DELETEITEM: {
                NMLISTVIEW& nmlv = *(NMLISTVIEW*)lparam;
                LVITEM lvi;
                lvi.mask = TVIF_PARAM;
                lvi.iItem = nmlv.iItem;
                lvi.iSubItem = 0;
                if (ListView_GetItem(nmlv.hdr.hwndFrom, &lvi))
                    delete (AutoPinRule*)lvi.lParam;
                return true;
            }
            case LVN_GETDISPINFO: {
                NMLVDISPINFO& di = *(NMLVDISPINFO*)lparam;
                const AutoPinRule* rule = (AutoPinRule*)di.item.lParam;
                if (!rule)
                    return false;
                // fill in the requested fields
                if (di.item.mask & LVIF_TEXT) {
                    //di.item.cchTextMax is 260..264 bytes
                    std::wstring s = rule->descr;
                    if (int(s.length()) > di.item.cchTextMax-1)
                        s = s.substr(0, di.item.cchTextMax-1);
                    wcscpy_s(di.item.pszText, di.item.cchTextMax, s.c_str());
                }
                if (di.item.mask & LVIF_STATE) {
                    di.item.stateMask = LVIS_STATEIMAGEMASK;
                    di.item.state = INDEXTOSTATEIMAGEMASK(rule->enabled ? 2 : 1);
                }
                return true;
            }
            case LVN_ITEMCHANGING: {
                NMLISTVIEW& nm = *(NMLISTVIEW*)lparam;
                AutoPinRule* rule = (AutoPinRule*)nm.lParam;
                if (nm.uChanged & LVIF_STATE && nm.uNewState & LVIS_STATEIMAGEMASK) {
                    rule->enabled = !rule->enabled;
                    ListView_Update(nm.hdr.hwndFrom, nm.iItem);
                }
                return true;
            }
            default:
                return false;
            }
        }
        case WM_COMMAND: {
            WORD id = LOWORD(wparam), code = HIWORD(wparam);
            switch (id) {
                case IDC_AUTOPIN_ON:    return cmAutoPinOn(wnd);
                case IDC_ADD: {
                    // 获取本地化的新规则描述文本
                    std::wstring newRuleDesc = LANG_MGR.getString(L"strings.new_rule_description");
                    if (newRuleDesc.empty() || newRuleDesc == L"strings.new_rule_description") {
                        // 回退到英文
                        newRuleDesc = L"<New>";
                    }
                    
                    AutoPinRule rule(newRuleDesc);
                    int res = Util::Res::LocalizedDialogBoxParam(WORD(IDD_EDIT_AUTOPIN_RULE), 
                            wnd, (DLGPROC)apEditRuleDlgProc, LPARAM(&rule));
                    if (res == IDOK) {
                        int i = rlist.addNew();
                        if (i >= 0 && rlist.setRule(i, rule))
                            rlist.selectSingleRule(i);
                        Window::psChanged(wnd);
                    }
                    return true;
                }
                case IDC_EDIT: {
                    AutoPinRule rule;
                    int i = rlist.getFirstSel();
                    if (i >= 0 && rlist.getRule(i, rule)) {
                        int res = Util::Res::LocalizedDialogBoxParam(WORD(IDD_EDIT_AUTOPIN_RULE), 
                            wnd, (DLGPROC)apEditRuleDlgProc, LPARAM(&rule));
                        if (res == IDOK) {
                            rlist.setRule(i, rule);
                            Window::psChanged(wnd);
                        }
                    }
                    return true;
                }
                case IDC_REMOVE: {
                    rlist.remove(rlist.getSel());
                    Window::psChanged(wnd);
                    return true;
                }
                case IDC_UP: {
                    rlist.moveSelUp();
                    Window::psChanged(wnd);
                    return true;
                }
                case IDC_DOWN: {
                    rlist.moveSelDown();
                    Window::psChanged(wnd);
                    return true;
                }
                case IDC_RULE_DELAY:
                    if (code == EN_CHANGE)
                        Window::psChanged(wnd);
                    return true;
                default:
                    return false;
            }
        }
        default:
            return false;
    }

}
