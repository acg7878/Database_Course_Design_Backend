#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class ClubApprovalController : public drogon::HttpController<ClubApprovalController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ClubApprovalController::submitApproval, "/club/approval/submit", Post);
    ADD_METHOD_TO(ClubApprovalController::approveClub, "/club/approval/{1}", Put);
    ADD_METHOD_TO(ClubApprovalController::getApprovalList, "/club/approval/list", Get);
    METHOD_LIST_END

    void submitApproval(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
    void approveClub(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int approvalId) const;
    void getApprovalList(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) const;
};