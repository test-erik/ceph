import { CommonModule } from '@angular/common';
import { NgModule } from '@angular/core';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { RouterModule } from '@angular/router';

import { NgbNavModule, NgbPopoverModule } from '@ng-bootstrap/ng-bootstrap';
import { ChartsModule } from 'ng2-charts';

import { SharedModule } from '~/app/shared/shared.module';
import { CephSharedModule } from '../shared/ceph-shared.module';
import { FeedbackComponent } from '../shared/feedback/feedback.component';
import { DeprecatedDashboardComponent } from './dashboard/dashboard.component';
import { HealthPieComponent } from './health-pie/health-pie.component';
import { HealthComponent } from './health/health.component';
import { InfoCardComponent } from './info-card/info-card.component';
import { InfoGroupComponent } from './info-group/info-group.component';
import { MdsSummaryPipe } from './mds-summary.pipe';
import { MgrSummaryPipe } from './mgr-summary.pipe';
import { MonSummaryPipe } from './mon-summary.pipe';
import { OsdSummaryPipe } from './osd-summary.pipe';

@NgModule({
  imports: [
    CephSharedModule,
    CommonModule,
    NgbNavModule,
    SharedModule,
    ChartsModule,
    RouterModule,
    NgbPopoverModule,
    FormsModule,
    ReactiveFormsModule
  ],

  declarations: [
    HealthComponent,
    DeprecatedDashboardComponent,
    MonSummaryPipe,
    OsdSummaryPipe,
    MgrSummaryPipe,
    MdsSummaryPipe,
    HealthPieComponent,
    InfoCardComponent,
    InfoGroupComponent,
    FeedbackComponent
  ]
})
export class DashboardModule {}
