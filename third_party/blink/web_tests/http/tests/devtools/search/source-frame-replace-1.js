// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests different types of search-and-replace in SourceFrame\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.addScriptTag('resources/search.js');

  await UI.viewManager.showView('sources');
  SourcesTestRunner.showScriptSource('search.js', didShowScriptSource);

  function didShowScriptSource(sourceFrame) {
    var searchConfig = new UI.SearchableView.SearchConfig('REPLACEME1', true, false);
    SourcesTestRunner.replaceAndDumpChange(sourceFrame, searchConfig, 'REPLACED', false);

    var searchConfig = new UI.SearchableView.SearchConfig('REPLACEME2', true, false);
    SourcesTestRunner.replaceAndDumpChange(sourceFrame, searchConfig, 'REPLACED', true);

    TestRunner.completeTest();
  }
})();
