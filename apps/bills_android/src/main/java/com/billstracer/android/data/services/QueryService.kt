package com.billstracer.android.data.services

import com.billstracer.android.model.QueryResult

interface QueryService {
    suspend fun queryYear(isoYear: String): QueryResult

    suspend fun queryMonth(isoMonth: String): QueryResult
}
