## index

+------------------+-----------------------------------------+
| alias            | symbol                                  |
+==================+=========================================+
| @code            | ../../jam/cast/code.cast                |
| @modules         | ../../jam/user-modules-info.md          |
| @juce            | ../../jam/cast/juce.cast                |
| @cmake           | ../../archetype/cli/cast/cmake.cast     |
| @installer       | ../../archetype/cli/cast/installer.cast |
| @project-info    | ../project-info.md                      |
| @signing         | signing.md                              |
| @ProjectInfo     | ../Source/generated/ProjectInfo.h       |
| @Entitlements    | ../entitlements.plist                   |
| @CMakeLists      | ../CMakeLists.txt                       |
| @semicolon       | ;                                       |
| @Distribution    | installer/mac/distribution.xml          |
| @Installer       | installer/win/installer.nsi             |
| @Gh              | ../gh.cmake                             |
| @Release         | ../RELEASE.md                           |
| @source          | source.cast                             |
| @identifiers     | identifiers.md                          |
| @Identifiers     | ../Source/generated/Identifiers.h       |
| @text            | text.md                                 |
| @banner          | banner.md                               |
| @binaryFiles     | files.md                                |
| @Text            | ../Source/generated/Text.h              |
| @Files           | ../Source/generated/Files.h             |
| @HashMaps        | ../Source/generated/HashMaps.h          |
| @GeneratedSource | ../Source/generated/Generated.cpp       |
| @Generated       | ../Source/generated/Generated.h         |
| @string          | juce::String                            |
+------------------+-----------------------------------------+

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
| Identifiers.h  | ```                                                                                      | Every Id:: name.                          |
|                | @file Identifiers.h                                                                      |                                           |
|                | @brief SCOUT manifest vocabulary.                                                        |                                           |
|                | ```                                                                                      |                                           |
+----------------+------------------------------------------------------------------------------------------+-------------------------------------------+
| Text.h         | ```                                                                                      | One string per failure, plus the success  |
|                | @file Text.h                                                                             | line.                                     |
|                | @brief scout message strings — one constant per failure, one for success.                |                                           |
|                | ```                                                                                      |                                           |
+----------------+------------------------------------------------------------------------------------------+-------------------------------------------+
| Files.h        | ```                                                                                      | File names scout reads.                   |
|                | @file Files.h                                                                            |                                           |
|                | @brief scout's own file names, referenced by the program.                                |                                           |
|                | ```                                                                                      |                                           |
+----------------+------------------------------------------------------------------------------------------+-------------------------------------------+
| HashMaps.h     | ```                                                                                      | Banner rows.                              |
|                | @file HashMaps.h                                                                         |                                           |
|                | @brief scout's banner rows.                                                              |                                           |
|                | ```                                                                                      |                                           |
+----------------+------------------------------------------------------------------------------------------+-------------------------------------------+
| Generated.h    | ```                                                                                      | Include umbrella for every generated      |
|                | @file Generated.h                                                                        | header.                                   |
|                | @brief scout's generated-header umbrella — includes every generated header.              |                                           |
|                | ```                                                                                      |                                           |
+----------------+------------------------------------------------------------------------------------------+-------------------------------------------+

## output

+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| list                                          | separator                 | structure                                     | file             |
+===============================================+===========================+===============================================+==================+
| - [list]: @project-info:cmake                 | - [list]: @semicolon      | @cmake:cmake                                  | @CMakeLists      |
|                                               |                           | - [description]: @headers:brief               |                  |
| - [list]: @project-info:project info          |                           |                                               |                  |
| - [list]: @signing:signing                    |                           |                                               |                  |
| - [list]: @project-info:architecture          | - [list]: @semicolon      | - [list]: @cmake:value                        |                  |
| - [list]: @project-info:release:stage=        | - [list]: @semicolon      | - [list]: @cmake:mac                          |                  |
| - [list]: @project-info:release:stage=linker  | - [list]: @semicolon      | - [list]: @cmake:mac                          |                  |
| - [list]: @project-info:debug:stage=          | - [list]: @semicolon      | - [list]: @cmake:mac                          |                  |
| - [list]: @project-info:release:stage=        | - [list]: @semicolon      | - [list]: @cmake:win                          |                  |
| - [list]: @project-info:release:stage=linker  | - [list]: @semicolon      | - [list]: @cmake:win                          |                  |
| - [list]: @project-info:debug:stage=          | - [list]: @semicolon      | - [list]: @cmake:win                          |                  |
| - [list]: @project-info:user module           |                           | - [list]: @cmake:module                       |                  |
| - [list]: @project-info:source glob           |                           | - [list]: @cmake:glob-pattern                 |                  |
| - [list]: @project-info:define                |                           | - [list]: @cmake:value                        |                  |
| - [list]: @project-info:include               |                           | - [list]: @cmake:value                        |                  |
| - [list]: @project-info:juce module           |                           | - [list]: @cmake:value                        |                  |
| - [list]: @project-info:user module           |                           | - [list]: @cmake:link                         |                  |
| - [list]: @project-info:binary                |                           | - [list]: @cmake:value                        |                  |
| - [list]: @project-info:pack                  |                           | - [list]: @cmake:pack-value                   |                  |
|                                               |                           | - strip: @cmake:strip                         |                  |
|                                               |                           | - codesign: @cmake:codesign                   |                  |
|                                               |                           | - notarize: @cmake:notarize                   |                  |
|                                               |                           | - install-directory: @cmake:install-directory |                  |
|                                               |                           | - install-copy: @cmake:install-copy           |                  |
|                                               |                           | - install-rename: @cmake:install-rename       |                  |
|                                               |                           | - postinstall: @cmake:postinstall             |                  |
|                                               |                           | - xattr: @cmake:xattr                         |                  |
|                                               |                           | - verify: @cmake:verify                       |                  |
|                                               |                           | - pkg-staging: @cmake:pkg-staging             |                  |
|                                               |                           | - pkgbuild: @cmake:pkgbuild                   |                  |
|                                               |                           | - productbuild: @cmake:productbuild           |                  |
|                                               |                           | - productsign: @cmake:productsign             |                  |
|                                               |                           | - makensis: @cmake:makensis                   |                  |
|                                               |                           | - staple: @cmake:staple                       |                  |
|                                               |                           | - qa-directory: @cmake:qa-directory           |                  |
|                                               |                           | - qa-copy: @cmake:qa-copy                     |                  |
|                                               |                           | - patched-juce: @juce:patched-juce            |                  |
| - [list]: @modules:submodule:module=patch     |                           | - [list]: @juce:patch                         |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| > - [list]: @signing:signing:type=entitlement |                           | @code:[xml]entitlements                       | @Entitlements    |
|                                               |                           | > - [list]: @code:entitlement                 |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| > - [list]: @project-info:project info        |                           | @code:namespace                               | @ProjectInfo     |
|                                               |                           | - macro: #pragma once                         |                  |
|                                               |                           | - name: ProjectInfo                           |                  |
|                                               |                           | - [description]: @headers:brief               |                  |
|                                               |                           | > - [list]: @code:constant                    |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| - [list]: @project-info:cmake                 |                           | @installer:distribution                       | @Distribution    |
| - [list]: @project-info:project info          |                           |                                               |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| - [list]: @project-info:project info          |                           | @installer:installer                          | @Installer       |
| - [list]: @project-info:cmake                 |                           |                                               |                  |
| - [list]: @signing:signing                    |                           |                                               |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| - [list]: @project-info:cmake                 |                           | @cmake:gh                                     | @Gh              |
| - [list]: @project-info:project info          |                           | - [list]: @cmake:pack-value                   |                  |
| - [list]: @project-info:pack                  |                           |                                               |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| - [list]: @project-info:cmake                 |                           | @installer:[no-banner]release                 | @Release         |
| - [list]: @project-info:release notes         |                           | - [list]: @installer:note                     |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| - [list]: @identifiers:identifiers            |                           | @code:namespace                               | @Identifiers     |
|                                               |                           | - macro: #pragma once                         |                  |
|                                               |                           | - generated:                                  |                  |
|                                               |                           | - name: Id                                    |                  |
|                                               |                           | - [description]: @headers:brief               |                  |
|                                               |                           | - [list]: @code:identifier                    |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| > - [list]: @identifiers:identifiers          |                           | @source:source                                | @GeneratedSource |
|                                               |                           | - file: Generated.h                           |                  |
|                                               |                           |                                               |                  |
|                                               |                           | @code:namespace                               |                  |
|                                               |                           | - name: Id                                    |                  |
|                                               |                           | > - [list]: @code:identifier-definition       |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| - [list]: @binaryFiles:files                  |                           | @code:namespace                               | @GeneratedSource |
|                                               |                           | - name: files                                 |                  |
|                                               |                           | - [list]: @code:identifier-definition         |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| > - [list]: @banner:banner                    | - [list]: @code:linebreak | @code:namespace                               | @GeneratedSource |
|                                               |                           | - name: map                                   |                  |
|                                               |                           |                                               |                  |
|                                               |                           | @code:hash-map-definition                     |                  |
|                                               |                           | - name: banner                                |                  |
|                                               |                           | - keyType: @string                            |                  |
|                                               |                           | - valueType: @string                          |                  |
|                                               |                           | > - [list]: @code:utf8-entry                  |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| > - [list]: @text:diagnostics                 |                           | @code:namespace                               | @Text            |
|                                               |                           | - macro: #pragma once                         |                  |
|                                               |                           | - name: text                                  |                  |
|                                               |                           | - [description]: @headers:brief               |                  |
|                                               |                           |                                               |                  |
|                                               |                           | @code:struct                                  |                  |
|                                               |                           | - name: Diagnostics                           |                  |
|                                               |                           | > - [list]: @code:char                        |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
| - [list]: @binaryFiles:files                  |                           | @code:namespace                               | @Files           |
|                                               |                           | - macro: #pragma once                         |                  |
|                                               |                           | - name: files                                 |                  |
|                                               |                           | - [description]: @headers:brief               |                  |
|                                               |                           | - [list]: @code:identifier                    |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+
|                                               | - [list]: @code:linebreak | @code:namespace                               | @HashMaps        |
|                                               |                           | - macro: #pragma once                         |                  |
|                                               |                           | - name: map                                   |                  |
|                                               |                           | - [description]: @headers:brief               |                  |
|                                               |                           |                                               |                  |
|                                               |                           | @code:hash-map                                |                  |
|                                               |                           | - name: banner                                |                  |
|                                               |                           | - keyType: @string                            |                  |
|                                               |                           | - valueType: @string                          |                  |
+-----------------------------------------------+---------------------------+-----------------------------------------------+------------------+

## output index

+---------------------------------------+-----------+-------------------------+------------+
| list                                  | separator | structure               | file       |
+=======================================+===========+=========================+============+
| - [list]: @headers:file=ProjectInfo.h |           | @code:line              | @Generated |
|                                       |           | - macro: #pragma once   |            |
|                                       |           |                         |            |
|                                       |           | - [list]: @code:include |            |
+---------------------------------------+-----------+-------------------------+------------+
| - [list]: @headers:file=Identifiers.h |           | @code:line              | @Generated |
|                                       |           | - macro: #pragma once   |            |
|                                       |           |                         |            |
|                                       |           | - [list]: @code:include |            |
+---------------------------------------+-----------+-------------------------+------------+
| - [list]: @headers:file=Text.h        |           | @code:line              | @Generated |
|                                       |           | - macro: #pragma once   |            |
|                                       |           |                         |            |
|                                       |           | - [list]: @code:include |            |
+---------------------------------------+-----------+-------------------------+------------+
| - [list]: @headers:file=Files.h       |           | @code:line              | @Generated |
|                                       |           | - macro: #pragma once   |            |
|                                       |           |                         |            |
|                                       |           | - [list]: @code:include |            |
+---------------------------------------+-----------+-------------------------+------------+
| - [list]: @headers:file=HashMaps.h    |           | @code:line              | @Generated |
|                                       |           | - macro: #pragma once   |            |
|                                       |           |                         |            |
|                                       |           | - [list]: @code:include |            |
+---------------------------------------+-----------+-------------------------+------------+
