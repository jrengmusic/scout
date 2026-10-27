## index

+--------------------+-----------------------------------------+
| alias              | symbol                                  |
+====================+=========================================+
| @code              | ../../jam/cast/code.cast                |
| @modules           | ../../jam/user-modules-info.md          |
| @juce              | ../../jam/cast/juce.cast                |
| @cmake             | ../../archetype/cli/cast/cmake.cast     |
| @installer         | ../../archetype/cli/cast/installer.cast |
| @project-info      | ../project-info.md                      |
| @signing           | signing.md                              |
| @ProjectInfo       | ../Source/generated/ProjectInfo.h       |
| @Entitlements      | ../entitlements.plist                   |
| @CMakeLists        | ../CMakeLists.txt                       |
| @semicolon         | ;                                       |
| @Distribution      | installer/mac/distribution.xml          |
| @Installer         | installer/win/installer.nsi             |
| @Gh                | ../gh.cmake                             |
| @Release           | ../RELEASE.md                           |
| @source            | source.cast                             |
| @identifiers       | identifiers.md                          |
| @Identifiers       | ../Source/generated/Identifiers.h       |
| @IdentifiersSource | ../Source/generated/Identifiers.cpp     |
+--------------------+-----------------------------------------+

## headers

+----------------+------------------------------------------------------------------------------------------+-------------------------------------------+
| file           | brief                                                                                    | description                               |
+================+==========================================================================================+===========================================+
| ProjectInfo.h  | ```                                                                                      | Product name, version, and source commit. |
|                | @file ProjectInfo.h                                                                      |                                           |
|                | @brief Project metadata — the generated ProjectInfo namespace.                           |                                           |
|                | ```                                                                                      |                                           |
+----------------+------------------------------------------------------------------------------------------+-------------------------------------------+
| CMakeLists.txt | ```                                                                                      |                                           |
|                | @file CMakeLists.txt                                                                     |                                           |
|                | @brief scout build manifest — a self-sufficient JUCE console project.                    |                                           |
|                |                                                                                          |                                           |
|                | Generated from project-info.md and cast/signing.md; every value traces to one table row. |                                           |
|                | Edit the table, run cast, then configure.                                                |                                           |
|                | ```                                                                                      |                                           |
+----------------+------------------------------------------------------------------------------------------+-------------------------------------------+
| Identifiers.h  | ```                                                                                      |                                           |
|                | @file Identifiers.h                                                                      |                                           |
|                | @brief SCOUT manifest vocabulary.                                                        |                                           |
|                | ```                                                                                      |                                           |
+----------------+------------------------------------------------------------------------------------------+-------------------------------------------+

## output

+-----------------------------------------------+----------------------+-----------------------------------------------+--------------------+
| list                                          | separator            | structure                                     | file               |
+===============================================+======================+===============================================+====================+
| - [list]: @project-info:cmake                 | - [list]: @semicolon | @cmake:cmake                                  | @CMakeLists        |
|                                               |                      | - [description]: @headers:brief               |                    |
| - [list]: @project-info:project info          |                      |                                               |                    |
| - [list]: @signing:signing                    |                      |                                               |                    |
| - [list]: @project-info:architecture          | - [list]: @semicolon | - [list]: @cmake:value                        |                    |
| - [list]: @project-info:release:stage=        | - [list]: @semicolon | - [list]: @cmake:mac                          |                    |
| - [list]: @project-info:release:stage=linker  | - [list]: @semicolon | - [list]: @cmake:mac                          |                    |
| - [list]: @project-info:debug:stage=          | - [list]: @semicolon | - [list]: @cmake:mac                          |                    |
| - [list]: @project-info:release:stage=        | - [list]: @semicolon | - [list]: @cmake:win                          |                    |
| - [list]: @project-info:release:stage=linker  | - [list]: @semicolon | - [list]: @cmake:win                          |                    |
| - [list]: @project-info:debug:stage=          | - [list]: @semicolon | - [list]: @cmake:win                          |                    |
| - [list]: @project-info:user module           |                      | - [list]: @cmake:module                       |                    |
| - [list]: @project-info:source glob           |                      | - [list]: @cmake:glob-pattern                 |                    |
| - [list]: @project-info:define                |                      | - [list]: @cmake:value                        |                    |
| - [list]: @project-info:include               |                      | - [list]: @cmake:value                        |                    |
| - [list]: @project-info:juce module           |                      | - [list]: @cmake:value                        |                    |
| - [list]: @project-info:user module           |                      | - [list]: @cmake:link                         |                    |
| - [list]: @project-info:binary                |                      | - [list]: @cmake:value                        |                    |
| - [list]: @project-info:pack                  |                      | - [list]: @cmake:pack-value                   |                    |
|                                               |                      | - strip: @cmake:strip                         |                    |
|                                               |                      | - codesign: @cmake:codesign                   |                    |
|                                               |                      | - notarize: @cmake:notarize                   |                    |
|                                               |                      | - install-directory: @cmake:install-directory |                    |
|                                               |                      | - install-copy: @cmake:install-copy           |                    |
|                                               |                      | - install-rename: @cmake:install-rename       |                    |
|                                               |                      | - postinstall: @cmake:postinstall             |                    |
|                                               |                      | - xattr: @cmake:xattr                         |                    |
|                                               |                      | - verify: @cmake:verify                       |                    |
|                                               |                      | - pkg-staging: @cmake:pkg-staging             |                    |
|                                               |                      | - pkgbuild: @cmake:pkgbuild                   |                    |
|                                               |                      | - productbuild: @cmake:productbuild           |                    |
|                                               |                      | - productsign: @cmake:productsign             |                    |
|                                               |                      | - makensis: @cmake:makensis                   |                    |
|                                               |                      | - staple: @cmake:staple                       |                    |
|                                               |                      | - qa-directory: @cmake:qa-directory           |                    |
|                                               |                      | - qa-copy: @cmake:qa-copy                     |                    |
|                                               |                      | - patched-juce: @juce:patched-juce            |                    |
| - [list]: @modules:submodule:module=patch     |                      | - [list]: @juce:patch                         |                    |
+-----------------------------------------------+----------------------+-----------------------------------------------+--------------------+
| > - [list]: @signing:signing:type=entitlement |                      | @code:[xml]entitlements                       | @Entitlements      |
|                                               |                      | > - [list]: @code:entitlement                 |                    |
+-----------------------------------------------+----------------------+-----------------------------------------------+--------------------+
| > - [list]: @project-info:project info        |                      | @code:namespace                               | @ProjectInfo       |
|                                               |                      | - macro: #pragma once                         |                    |
|                                               |                      | - name: ProjectInfo                           |                    |
|                                               |                      | - [description]: @headers:brief               |                    |
|                                               |                      | > - [list]: @code:constant                    |                    |
+-----------------------------------------------+----------------------+-----------------------------------------------+--------------------+
| - [list]: @project-info:cmake                 |                      | @installer:distribution                       | @Distribution      |
| - [list]: @project-info:project info          |                      |                                               |                    |
+-----------------------------------------------+----------------------+-----------------------------------------------+--------------------+
| - [list]: @project-info:project info          |                      | @installer:installer                          | @Installer         |
| - [list]: @project-info:cmake                 |                      |                                               |                    |
| - [list]: @signing:signing                    |                      |                                               |                    |
+-----------------------------------------------+----------------------+-----------------------------------------------+--------------------+
| - [list]: @project-info:cmake                 |                      | @cmake:gh                                     | @Gh                |
| - [list]: @project-info:project info          |                      | - [list]: @cmake:pack-value                   |                    |
| - [list]: @project-info:pack                  |                      |                                               |                    |
+-----------------------------------------------+----------------------+-----------------------------------------------+--------------------+
| - [list]: @project-info:cmake                 |                      | @installer:[no-banner]release                 | @Release           |
| - [list]: @project-info:release notes         |                      | - [list]: @installer:note                     |                    |
+-----------------------------------------------+----------------------+-----------------------------------------------+--------------------+
| - [list]: @identifiers:identifiers            |                      | @code:namespace                               | @Identifiers       |
|                                               |                      | - macro: #pragma once                         |                    |
|                                               |                      | - generated:                                  |                    |
|                                               |                      | - name: Id                                    |                    |
|                                               |                      | - [description]: @headers:brief               |                    |
|                                               |                      | - [list]: @code:identifier                    |                    |
+-----------------------------------------------+----------------------+-----------------------------------------------+--------------------+
| > - [list]: @identifiers:identifiers          |                      | @source:source                                | @IdentifiersSource |
|                                               |                      | - file: Identifiers.h                         |                    |
|                                               |                      |                                               |                    |
|                                               |                      | @code:namespace                               |                    |
|                                               |                      | - name: Id                                    |                    |
|                                               |                      | > - [list]: @code:identifier-definition       |                    |
+-----------------------------------------------+----------------------+-----------------------------------------------+--------------------+
